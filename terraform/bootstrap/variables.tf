variable "aws_region" {
  description = "AWS region to create backend resources in"
  type        = string
  default     = "us-east-1"
}

variable "project_name" {
  description = "Project name — used as prefix for bucket and table names"
  type        = string
  default     = "incident-backend"
}
