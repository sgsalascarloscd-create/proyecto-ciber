# ────────────────────────────────────────────────────────────
# Bootstrap — Terraform remote state backend infrastructure
# ────────────────────────────────────────────────────────────
# Run this FIRST, before any other terraform in this repo.
#
#   cd terraform/bootstrap
#   terraform init
#   terraform apply
#
# After it completes, uncomment the `backend "s3" {}` block
# in ../main.tf and run `terraform init -reconfigure` to
# migrate the main state from local to S3.
# ────────────────────────────────────────────────────────────

terraform {
  required_version = ">= 1.5"

  required_providers {
    aws = {
      source  = "hashicorp/aws"
      version = "~> 5.0"
    }
  }
}

provider "aws" {
  region = var.aws_region
}

# ── S3 bucket ──────────────────────────────────────────────
# Stores all terraform state files for this project.
# Versioning keeps a history of every state change.
# Encryption and public-access blocks are mandatory.

resource "aws_s3_bucket" "state" {
  bucket = "${var.project_name}-tfstate"

  # Prevent accidental deletion in prod; allow in non-prod
  force_destroy = false

  tags = {
    Name    = "${var.project_name}-tfstate"
    Purpose = "Terraform remote state storage"
  }
}

resource "aws_s3_bucket_versioning" "state" {
  bucket = aws_s3_bucket.state.id

  versioning_configuration {
    status = "Enabled"
  }
}

resource "aws_s3_bucket_server_side_encryption_configuration" "state" {
  bucket = aws_s3_bucket.state.id

  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}

resource "aws_s3_bucket_public_access_block" "state" {
  bucket = aws_s3_bucket.state.id

  block_public_acls       = true
  block_public_policy     = true
  ignore_public_acls      = true
  restrict_public_buckets = true
}

# ── DynamoDB table ─────────────────────────────────────────
# Used for state locking — prevents concurrent operations.

resource "aws_dynamodb_table" "state_lock" {
  name         = "${var.project_name}-tfstate-lock"
  billing_mode = "PAY_PER_REQUEST"
  hash_key     = "LockID"

  attribute {
    name = "LockID"
    type = "S"
  }

  tags = {
    Name    = "${var.project_name}-tfstate-lock"
    Purpose = "Terraform state lock"
  }
}
