# FLEDGE K/V Server developer guide

## Data Server

The data server provides the read API for the KV service.

### How to Run

#### Prereqs

- Install Bazelisk is the recommended way to
    [install Bazel](https://docs.bazel.build/versions/5.0.0/install-bazelisk.html) or refer to
    Bazelisk's
    [installation instructions](https://github.com/bazelbuild/bazelisk/blob/master/README.md#installation).
    Hint: If you have Go installed, then `go install` may be the simplest option, per
    [these instructions](https://github.com/bazelbuild/bazelisk/blob/master/README.md#requirements).
    on linux, Windows, and macOS.

    Note: These instructions refer to the `bazel` command. Therefore, if using bazelisk instead,
    simply specify `bazelisk` instead of `bazel`, add an alias for `bazel`, or symlink `bazelisk` to
    `bazel` and add this to your `PATH`.

    If installing bazel directly rather than using bazelisk, refer to the `.bazeliskrc` file for the
    matching bazel version.

- Optional steps:

  - Install [Docker](https://docs.docker.com/get-docker/)
  - Install
        [grpc_cli](https://github.com/grpc/grpc/blob/master/doc/server_reflection_tutorial.md#test-services-using-server-reflection)
        , if you do not already have it

```sh
bazel build @com_github_grpc_grpc//test/cpp/util:grpc_cli
cp "$(bazel info bazel-bin)/external/com_github_grpc_grpc/test/cpp/util/grpc_cli" /bin/opt/grpc_cli
```

#### Running the server locally

For example:

```sh
bazel run //components/data_server/server:server --//:parameters=local --//:platform=aws -- --environment="dev"
```

> Attention: The server can run locally while specifying `aws` as platform, in which case it will contact AWS based on the local AWS credentials. However, this requires the AWS environment to be set up first following the [AWS deployment guide](/docs/deploying_on_aws.md).

We are currently developing this server for local testing and for use on AWS Nitro instances
(similar to the
[Aggregation Service](https://github.com/google/trusted-execution-aggregation-service). We
anticipate supporting additional cloud providers in the future.

#### Interacting with the server

Use `grpc_cli` to interact with your local instance. You might have to pass
`--channel_creds_type=insecure`.

Example:

```sh
grpc_cli call localhost:50051 GetValues "kv_internal: 'hi'" --channel_creds_type=insecure
```

## Develop and run the server inside AWS enclave

The KV service instance should be set up by following the deployment guide
([AWS](/docs/deploying_on_aws.md)). For faster iteration, enclave image of the server is also
produced under `dist/`. Once the system has been started, iterating on changes to the server itself
only requires restarting the enclave image:

1. Copy the new enclave EIF to an AWS EC2 instance that supports nitro enclave. Note: The system has
   a SSH instance that a developer can access. From there the user can access actual server EC2
   instances, using the same SSH key. So the copy command below should be repeated twice to reach
   the destination EC2 instance.

    ```sh
    scp -i ~/"key.pem" dist/server_enclave_image.eif ec2-user@${EC2_ADDR}.compute-1.amazonaws.com:/home/ec2-user/server_enclave_image.eif
    ```

1. Start the enclave job (If one is running, terminate it first, see below for instructions):

    ```sh
    nitro-cli run-enclave --cpu-count 2 --memory 30720 --eif-path server_enclave_image.eif --debug-mode --enclave-cid 16
    ```

1. To see logs of the TEE job:

    ```sh
    ENCLAVE_ID=$(nitro-cli describe-enclaves | jq -r ".[0].EnclaveID"); [ "$ENCLAVE_ID" != "null" ] && nitro-cli console --enclave-id ${ENCLAVE_ID}
    ```

1. To terminate the job:

    ```sh
    ENCLAVE_ID=$(nitro-cli describe-enclaves | jq -r ".[0].EnclaveID"); [ "$ENCLAVE_ID" != "null" ] && nitro-cli terminate-enclave --enclave-id ${ENCLAVE_ID}
    ```

### Specifying platform specific src/dep

It's possible to use polymorphism + build-time flag to only build and link code specific to a
platform.

Example:

```build
cc_library(
    name = "blob_storage_client",
    srcs = select({
        "//:aws_platform": ["s3_blob_storage_client.cc"],
    }),
    hdrs = [
        "blob_storage_client.h",
    ],
    deps = select({
        "//:aws_platform": ["@aws_sdk_cpp//:s3"],
    }) + [
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
    ],
)
```

Available conditions are:

- //:aws_platform
- //:local_platform

Parameters can be configured separately to be read from specific platforms.

- //:aws_parameters
- //:local_parameters
