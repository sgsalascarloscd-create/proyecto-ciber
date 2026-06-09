variable "aws_region" {
  description = "AWS region to deploy into"
  type        = string
  default     = "us-east-1"
}

variable "project_name" {
  description = "Name of the project"
  type        = string
  default     = "incident-backend"
}

variable "environment" {
  description = "Deployment environment"
  type        = string
  default     = "dev"
}

variable "vpc_cidr" {
  description = "CIDR block for the VPC"
  type        = string
  default     = "10.0.0.0/16"
}

variable "public_subnet_cidrs" {
  description = "CIDR blocks for public subnets"
  type        = list(string)
  default     = ["10.0.1.0/24", "10.0.2.0/24"]
}

variable "container_port" {
  description = "Port the container listens on"
  type        = number
  default     = 8080
}

variable "health_check_path" {
  description = "Path for ALB health checks"
  type        = string
  default     = "/api/v1/incidents"
}

variable "task_cpu" {
  description = "CPU units for the Fargate task (256 = 0.25 vCPU)"
  type        = string
  default     = "256"
}

variable "task_memory" {
  description = "Memory for the Fargate task in MiB"
  type        = string
  default     = "512"
}

variable "desired_count" {
  description = "Number of desired ECS task instances"
  type        = number
  default     = 1
}

variable "image_tag" {
  description = "Tag of the Docker image in ECR"
  type        = string
  default     = "latest"
}

variable "db_path" {
  description = "Path to the SQLite database file inside the container"
  type        = string
  default     = "/data/incidents.db"
}

variable "log_level" {
  description = "Log level for the application"
  type        = string
  default     = "info"
}

variable "log_retention_days" {
  description = "Number of days to retain CloudWatch logs"
  type        = number
  default     = 30
}

variable "index_document" {
  description = "Index document for the S3 website"
  type        = string
  default     = "index.html"
}

variable "error_document" {
  description = "Error document for the S3 website"
  type        = string
  default     = "error.html"
}
