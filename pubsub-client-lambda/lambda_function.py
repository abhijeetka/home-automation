import json
import boto3

client = boto3.client('iot-data')
print('Loading function')


def lambda_handler(event, context):
    print(event)
    
    if(event["httpMethod"] == "GET"):
        deviceName = event['queryStringParameters']['deviceName']
        state = event['queryStringParameters']['newState']
        response = client.publish(
            topic = "esp32/sub",
            qos = 1,
            payload = json.dumps({"state":{"reported":{"deviceName": deviceName,"state": state }}})
        
        )
        return {
            "statusCode" : 200,
        }
    elif(event["httpMethod"] == "POST"):
        body = json.loads(event["body"])
        deviceName = body["deviceName"]
        state = body["newState"]
        response = client.publish(
            topic = "esp32/sub",
            qos = 1,
            payload = json.dumps({"state":{"reported":{"deviceName": deviceName,"state": state }}})
        
        )
        return {
            "statusCode" : 200,
        }
        
        
