# Zip the Lambda function code
data "archive_file" "lambda_zip" {
  type        = "zip"
  source_file = "${path.module}/../lambda/lambda_function.py"
  output_path = "${path.module}/bootstrap/lambda_function.zip"
}

# IAM Role for Lambda
resource "aws_iam_role" "lambda_exec" {
  name = "${local.name_prefix}-lambda-exec-role"

  assume_role_policy = jsonencode({
    Version = "2012-10-17"
    Statement = [
      {
        Action = "sts:AssumeRole"
        Effect = "Allow"
        Principal = {
          Service = "lambda.amazonaws.com"
        }
      }
    ]
  })
}

# IAM Policy to execute Lambda and write CloudWatch logs
resource "aws_iam_role_policy" "lambda_exec_policy" {
  name = "${local.name_prefix}-lambda-exec-policy"
  role = aws_iam_role.lambda_exec.id

  policy = jsonencode({
    Version = "2012-10-17"
    Statement = [
      {
        Action = [
          "logs:CreateLogGroup",
          "logs:CreateLogStream",
          "logs:PutLogEvents"
        ]
        Effect   = "Allow"
        Resource = "arn:aws:logs:*:*:*"
      }
    ]
  })
}

# Lambda Function definition
resource "aws_lambda_function" "soc_parser" {
  filename         = data.archive_file.lambda_zip.output_path
  function_name    = "${local.name_prefix}-parser-function"
  role             = aws_iam_role.lambda_exec.arn
  handler          = "lambda_function.lambda_handler"
  source_code_hash = data.archive_file.lambda_zip.output_base64sha256
  runtime          = "python3.10"
  timeout          = 10

  environment {
    variables = {
      # Points to the application load balancer URL dynamically created by ecs.tf
      SOC_BACKEND_URL = "http://${aws_lb.main.dns_name}/api/v1/aws/events"
      WEBHOOK_SECRET  = "lambda-soc-webhook-secret-token-key-2026"
    }
  }
}

# Allow EventBridge to invoke our Lambda function
resource "aws_lambda_permission" "allow_eventbridge_to_invoke_lambda" {
  statement_id  = "AllowExecutionFromEventBridge"
  action        = "lambda:InvokeFunction"
  function_name = aws_lambda_function.soc_parser.function_name
  principal     = "events.amazonaws.com"
  source_arn    = aws_cloudwatch_event_rule.security_events.arn
}
