load(
    "@io_bazel_rules_docker//container:container.bzl",
    "container_image",
    "container_layer",
    "container_push",
)

py_library(
    name = "sqs_cleanup_manager",
    srcs = ["sqs_cleanup_manager.py"],
)

py_test(
    name = "sqs_cleanup_manager_test",
    srcs = ["sqs_cleanup_manager_test.py"],
    python_version = "PY3",
    deps = [
        ":sqs_cleanup_manager",
    ],
)

container_layer(
    name = "lambda_binary_layer",
    directory = "/var/task",  # ${LAMBDA_TASK_ROOT}
    files = [
        "sqs_cleanup.py",
        "sqs_cleanup_manager.py",
    ],
)

container_image(
    name = "sqs_lambda",
    base = "@aws-lambda-python//image",
    cmd = ["sqs_cleanup.handler"],
    layers = [
        ":lambda_binary_layer",
    ],
)

container_push(
    name = "sqs_lambda_push_aws_ecr",
    format = "Docker",
    image = ":sqs_lambda",
    registry = "$${AWS_ECR}",
    repository = "sqs_lambda",
    tag = "latest",
)
