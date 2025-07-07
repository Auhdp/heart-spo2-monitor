<?php
$hostname = "localhost";
$username = "root";
$password = "";
$database = "max30100";

$conn = mysqli_connect($hostname, $username, $password, $database);
if (!$conn) {
    die("Connection failed: " . mysqli_connect_error());
}

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    // Nhận dữ liệu từ yêu cầu POST
    $postData = json_decode(file_get_contents("php://input"));

    if ($postData) {
        $heartRate = $postData->heartRate;
        $spo2 = $postData->spo2;

        if ($heartRate != 0 && $spo2 != 0) {
            $sql = "INSERT INTO data_max (heartRate, spo2) VALUES (?, ?)";
            $stmt = mysqli_prepare($conn, $sql);
            mysqli_stmt_bind_param($stmt, 'dd', $heartRate, $spo2);

            if (mysqli_stmt_execute($stmt)) {
                $last_id = mysqli_insert_id($conn);
                echo "Data inserted successfully. ID: " . $last_id;
            } else {
                echo "Error: " . mysqli_error($conn);
            }

            mysqli_stmt_close($stmt);
        } else {
            echo "Heart Rate and SpO2 must be greater than 0 to save to the database.";
        }
    } else {
        echo "Invalid POST data";
    }
} else if ($_SERVER['REQUEST_METHOD'] === 'GET') {
    // Truy vấn dữ liệu từ cơ sở dữ liệu và trả về dưới dạng JSON
    $sql = "SELECT * FROM data_max";
    $result = mysqli_query($conn, $sql);

    $rows = array();
    while ($row = mysqli_fetch_assoc($result)) {
        $rows[] = $row;
    }

    header('Content-Type: application/json');
    echo json_encode($rows);
} else {
    echo "Invalid request method";
}

mysqli_close($conn);
?>
