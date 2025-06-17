import json
import boto3
from botocore.exceptions import NoCredentialsError
from datetime import datetime

client = boto3.client("iot-data", region_name="us-east-1")
bucket_name = "YOUR_BUCKET_ID"
# devices publish to $aws/rules/GetUploadURL/DeviceId
# They publish a message with the file name they want to upload {"file": "file.jpg"}


def lambda_handler(event, context):
    print("Got IoT event")
    print(event)
    print("Got IoT context")
    print(context)

    file_for_upload = event["message"]["file"]
    print(f"File for upload: {file_for_upload}")
    incoming_topic = event["topic"]
    print(f"Incoming topic: {incoming_topic}")
    device_id = incoming_topic.split("/")[-1]
    print(f"Parsed Device ID: {device_id}")

    try:
        presigned_url = generate_presigned_url(bucket_name, file_for_upload)
        if presigned_url:
            print(f"The url successfully generated: {presigned_url}")
            out_going_message = {
                "file": file_for_upload,
                "url": presigned_url,
                "timestamp": datetime.now().isoformat(),
            }
            print(f"Outgoing message: {out_going_message}")
            # return publishToIoT(f"{device_id}/upload_url", out_going_message)
            return publishToIoT(f"{device_id}/upload_url", presigned_url)
        else:
            return publishToIoT(
                f"{device_id}/upload_url", "Failed to generate pre-signed URL."
            )
    except Exception as e:
        return publishToIoT(
            f"{device_id}/upload_url", "Failed to generate pre-signed URL."
        )


def generate_presigned_url(bucket_name, object_name, expiration_time=3600):
    s3_client = boto3.client("s3")
    try:
        response = s3_client.generate_presigned_url(
            "put_object",
            Params={
                "Bucket": bucket_name,
                "Key": object_name,
                "ContentType": "image/jpeg",
            },
            ExpiresIn=expiration_time,
        )
    except NoCredentialsError:
        print("Credentials not available")
        return None

    return response


def publishToIoT(topic, payload):
    # response = client.publish(topic=topic, qos=1, payload=json.dumps(payload))
    # NOTE: Use QOS 0 for now - fire and forget
    response = client.publish(topic=topic, qos=0, payload=payload)
    print(f"Published message to {topic}: {payload}")
    print("Response: ", response)
    return response
