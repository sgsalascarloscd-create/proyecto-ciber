# EventBridge Rule to match security events of interest
resource "aws_cloudwatch_event_rule" "security_events" {
  name        = "${local.name_prefix}-security-events-rule"
  description = "Capture high-risk security operations from S3, IAM, STS, EC2, and VPC"

  # Pattern to match specific services
  event_pattern = jsonencode({
    source = [
      "aws.iam",
      "aws.s3",
      "aws.ec2",
      "aws.sts",
      "aws.vpc"
    ]
    "detail-type" = [
      "AWS API Call via CloudTrail",
      "AWS Console Sign-In via CloudTrail"
    ]
  })
}

# Link EventBridge to AWS Lambda Target
resource "aws_cloudwatch_event_target" "lambda_target" {
  rule      = aws_cloudwatch_event_rule.security_events.name
  target_id = "SendToSOCParserLambda"
  arn       = aws_lambda_function.soc_parser.arn
}
