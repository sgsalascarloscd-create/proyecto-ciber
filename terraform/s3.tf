resource "aws_s3_bucket" "web" {
  bucket = "${local.name_prefix}-web"

  force_destroy = true

  tags = {
    Name = "${local.name_prefix}-web"
  }
}

resource "aws_s3_bucket_website_configuration" "web" {
  bucket = aws_s3_bucket.web.id

  index_document {
    suffix = var.index_document
  }

  error_document {
    key = var.error_document
  }
}

resource "aws_s3_bucket_public_access_block" "web" {
  bucket = aws_s3_bucket.web.id

  block_public_acls       = false
  block_public_policy     = false
  ignore_public_acls      = false
  restrict_public_buckets = false
}

resource "aws_s3_bucket_policy" "web" {
  bucket = aws_s3_bucket.web.id
  policy = data.aws_iam_policy_document.web.json
}

data "aws_iam_policy_document" "web" {
  statement {
    principals {
      type        = "AWS"
      identifiers = ["*"]
    }

    actions = [
      "s3:GetObject",
    ]

    resources = [
      "${aws_s3_bucket.web.arn}/*",
    ]
  }
}

# NOTE: index.html is uploaded by the CI/CD pipeline (deploy.yml → sync-s3 job).
# The pipeline injects the live ALB DNS into the BASE constant before upload.
# This keeps the repo's index.html with the localhost default for dev use.
