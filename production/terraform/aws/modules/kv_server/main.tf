/**
 * Copyright 2022 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

locals {
  service = "kv-server"
}

module "iam_roles" {
  source      = "../../services/iam_roles"
  environment = var.environment
  service     = local.service
}

module "data_storage" {
  source                    = "../../services/data_storage"
  environment               = var.environment
  service                   = local.service
  s3_delta_file_bucket_name = var.s3_delta_file_bucket_name
  bucket_notification_dependency = [module.sqs_cleanup.allow_sqs_cleanup_execution_as_dependency]
}

module "sqs_cleanup" {
  source                     = "../../services/sqs_cleanup"
  environment                = var.environment
  service                    = local.service
  sqs_cleanup_image_uri      = var.sqs_cleanup_image_uri
  lambda_role_arn            = module.iam_roles.lambda_role_arn
  sqs_cleanup_schedule       = var.sqs_cleanup_schedule
  sns_data_updates_topic_arn = module.data_storage.sns_data_updates_topic_arn
  sqs_queue_timeout_secs     = var.sqs_queue_timeout_secs
}

module "networking" {
  source         = "../../services/networking"
  service        = local.service
  environment    = var.environment
  vpc_cidr_block = var.vpc_cidr_block
}

module "security_groups" {
  source      = "../../services/security_groups"
  environment = var.environment
  service     = local.service
  vpc_id      = module.networking.vpc_id
}

module "backend_services" {
  source                          = "../../services/backend_services"
  region                          = var.region
  environment                     = var.environment
  service                         = local.service
  vpc_endpoint_route_table_ids    = module.networking.private_route_table_ids
  vpc_endpoint_sg_id              = module.security_groups.vpc_endpoint_security_group_id
  vpc_endpoint_subnet_ids         = module.networking.private_subnet_ids
  vpc_gateway_endpoint_services   = var.vpc_gateway_endpoint_services
  vpc_id                          = module.networking.vpc_id
  vpc_interface_endpoint_services = var.vpc_interface_endpoint_services
  server_instance_role_arn        = module.iam_roles.instance_role_arn
}

module "load_balancing" {
  source                          = "../../services/load_balancing"
  environment                     = var.environment
  service                         = local.service
  certificate_arn                 = var.certificate_arn
  elb_subnet_ids                  = module.networking.public_subnet_ids
  server_port                     = var.server_port
  vpc_id                          = module.networking.vpc_id
  elb_security_group_id           = module.security_groups.elb_security_group_id
  root_domain                     = var.root_domain
  root_domain_zone_id             = var.root_domain_zone_id
  healthcheck_healthy_threshold   = var.healthcheck_healthy_threshold
  healthcheck_interval_sec        = var.healthcheck_interval_sec
  healthcheck_unhealthy_threshold = var.healthcheck_unhealthy_threshold
}

module "autoscaling" {
  source                       = "../../services/autoscaling"
  environment                  = var.environment
  service                      = local.service
  autoscaling_subnet_ids       = module.networking.private_subnet_ids
  instance_ami_id              = var.instance_ami_id
  instance_security_group_id   = module.security_groups.instance_security_group_id
  instance_type                = var.instance_type
  target_group_arns            = module.load_balancing.target_group_arns
  autoscaling_desired_capacity = var.autoscaling_desired_capacity
  autoscaling_max_size         = var.autoscaling_max_size
  autoscaling_min_size         = var.autoscaling_min_size
  instance_profile_arn         = module.iam_roles.instance_profile_arn
  instance_ssh_key_name        = var.instance_ssh_key_name
  enclave_cpu_count            = var.enclave_cpu_count
  enclave_memory_mib           = var.enclave_memory_mib
  server_port                  = var.server_port
}

module "ssh" {
  source                  = "../../services/ssh"
  environment             = var.environment
  instance_sg_id          = module.security_groups.ssh_security_group_id
  instance_ssh_key_name   = var.instance_ssh_key_name
  service                 = local.service
  ssh_instance_subnet_ids = module.networking.public_subnet_ids
}

module "parameter" {
  source                                = "../../services/parameter"
  service                               = local.service
  environment                           = var.environment
  mode_parameter_value                  = var.mode
  s3_bucket_parameter_value             = module.data_storage.s3_data_bucket_id
  bucket_update_sns_arn_parameter_value = module.data_storage.sns_data_updates_topic_arn
}

module "security_group_rules" {
  source                            = "../../services/security_group_rules"
  service                           = local.service
  environment                       = var.environment
  server_instance_port              = var.server_port
  vpc_id                            = module.networking.vpc_id
  elb_security_group_id             = module.security_groups.elb_security_group_id
  instances_security_group_id       = module.security_groups.instance_security_group_id
  ssh_security_group_id             = module.security_groups.ssh_security_group_id
  vpce_security_group_id            = module.security_groups.vpc_endpoint_security_group_id
  gateway_endpoints_prefix_list_ids = module.backend_services.gateway_endpoints_prefix_list_ids
}

module "iam_role_policies" {
  source                       = "../../services/iam_role_policies"
  service                      = local.service
  environment                  = var.environment
  server_instance_role_name    = module.iam_roles.instance_role_name
  sqs_cleanup_lambda_role_name = module.iam_roles.lambda_role_name
  s3_delta_file_bucket_arn     = module.data_storage.s3_data_bucket_arn
  sns_data_updates_topic_arn   = module.data_storage.sns_data_updates_topic_arn
  server_parameter_arns = [
    module.parameter.mode_parameter_arn,
    module.parameter.s3_bucket_parameter_arn,
    module.parameter.bucket_update_sns_arn_parameter_arn
  ]
}
