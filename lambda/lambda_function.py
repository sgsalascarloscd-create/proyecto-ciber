import os
import json
import hmac
import hashlib
import urllib.request
import urllib.parse

def lambda_handler(event, context):
    """
    AWS Lambda handler designed to receive CloudTrail events from Amazon EventBridge,
    normalize the payload, sign it with HMAC-SHA256, and webhook it to the SOC backend.
    """
    print("Received event: " + json.dumps(event))
    
    # 1. Parse Event Details from EventBridge wrapper
    detail = event.get("detail", {})
    if not detail:
        print("Empty detail block in EventBridge record.")
        return {"statusCode": 400, "body": "Invalid EventBridge record structure"}
    
    # 2. Retrieve config from Environment Variables
    backend_url = os.environ.get("SOC_BACKEND_URL", "http://YOUR_ALB_DNS/api/v1/aws/events")
    webhook_secret = os.environ.get("WEBHOOK_SECRET", "lambda-soc-webhook-secret-token-key-2026")
    
    # 3. Extract relevant parameters
    normalized_payload = {
        "eventID": detail.get("eventID"),
        "eventName": detail.get("eventName"),
        "eventSource": detail.get("eventSource"),
        "eventTime": detail.get("eventTime"),
        "sourceIPAddress": detail.get("sourceIPAddress"),
        "userAgent": detail.get("userAgent"),
        "userIdentity": detail.get("userIdentity", {}),
        "requestParameters": detail.get("requestParameters", {}),
        "responseElements": detail.get("responseElements", {})
    }
    
    payload_str = json.dumps(normalized_payload)
    
    # 4. Sign payload with HMAC-SHA256
    signature = hmac.new(
        webhook_secret.encode('utf-8'),
        payload_str.encode('utf-8'),
        hashlib.sha256
    ).hexdigest()
    
    signature_header = f"sha256={signature}"
    
    # 5. Forward payload to C++ backend
    req = urllib.request.Request(
        backend_url,
        data=payload_str.encode('utf-8'),
        headers={
            "Content-Type": "application/json",
            "X-SOC-Signature": signature_header
        },
        method="POST"
    )
    
    try:
        with urllib.request.urlopen(req, timeout=5) as response:
            response_body = response.read().decode('utf-8')
            print(f"Backend Response status={response.status}, body={response_body}")
            return {
                "statusCode": 200,
                "body": json.dumps({
                    "message": "Event forwarded successfully",
                    "backend_response": response_body
                })
            }
    except urllib.error.URLError as e:
        print(f"Failed to post to backend at {backend_url}: {str(e)}")
        # Return success (200) to EventBridge to avoid excessive retries, but log error
        return {
            "statusCode": 502,
            "body": f"Error posting to SOC backend: {str(e)}"
        }
