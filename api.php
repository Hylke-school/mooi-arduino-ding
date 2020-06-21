<?php 
// This API allows Arduino to send HTTPS FCM messages
// Takes raw data from the incoming HTTP request
$json = file_get_contents('php://input');

$curl = curl_init();
curl_setopt($curl, CURLOPT_URL, 'https://fcm.googleapis.com/fcm/send');
curl_setopt($curl, CURLOPT_POST, 1);
curl_setopt($curl, CURLOPT_POSTFIELDS, $json);

// build the outgoing headers
$hdr_out = array();
$hdr_out[] = 'Content-Type: application/json';
// read incoming header to extract just the auth key
foreach (getallheaders() as $name => $value) { 
    //echo "$name: $value\n"; 
    if (strtoupper($name) == "AUTHORIZATION") {
        $hdr_out[] = 'Authorization: '. $value;
    }
} 
curl_setopt($curl, CURLOPT_HTTPHEADER, $hdr_out);

$result = curl_exec($curl);

if ($result) {
    $response_code= curl_getinfo($curl, CURLINFO_RESPONSE_CODE);
    curl_close($curl);
    http_response_code($response_code);
}
else {
    echo "API Failure";
    http_response_code(500);
}
?>