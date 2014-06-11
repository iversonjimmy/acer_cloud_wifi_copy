<html>
<head>
    <title>GVM Test Runs</title>
    <link rel="stylesheet" type="text/css" href="style.css">
</head>
<body>

<?php

# get common values and functions
require("testharness_functions.php");

# header
require("site_header.php");

# connect to the database
require("connect_to_db_v1.php");

echo "<p>\n";
echo "<table border=1>\n";
echo "<tr>\n";
echo "    <th>Run ID</th>\n";
echo "    <th>Status</th>\n";
echo "    <th>Run Type</th>\n";
echo "    <th>Start Time</th>\n";
echo "    <th>End Time</th>\n";
echo "    <th>Elapsed Time</th>\n";
echo "    <th>Total</th>\n";
echo "    <th>Pass</th>\n";
echo "    <th>Fail</th>\n";
echo "    <th>Skip</th>\n";
echo "    <th>Indeterminate</th>\n";
echo "    <th>Remaining</th>\n";
echo "    <th>Log Dir</th>\n";
echo "</tr>\n";

# get test run info
$query = "SELECT * FROM test_run ORDER BY id DESC";
$result = mysql_query($query);
$num_rows = mysql_numrows($result);

$i=0;
while ( $i < $num_rows ) {
    $run_id = mysql_result($result,$i,"id");
    $run_name = mysql_result($result,$i,"name");
    $run_status = mysql_result($result,$i,"status");
    $run_start = mysql_result($result,$i,"start_time");
    $run_end = mysql_result($result,$i,"end_time");
    $tc_count = mysql_result($result,$i,"tc_count");
    $tc_pass = mysql_result($result,$i,"tc_pass");
    $tc_fail = mysql_result($result,$i,"tc_fail");
    $tc_skip = mysql_result($result,$i,"tc_skip");
    $tc_indeterminate = mysql_result($result,$i,"tc_indeterminate");
    $tc_remaining = mysql_result($result,$i,"tc_remaining");
    $output_dir = mysql_result($result,$i,"output_dir");

    # add color to the status
    if ( $run_status == "PASS" ) {
        $run_status = $pass_txt_style . $run_status . $style_end;
    }
    elseif ( $run_status == "FAIL" ) {
        $run_status = $fail_txt_style . $run_status . $style_end;
    }

    # calculate an elapsed time
    if ( $run_start == "" || $run_end == "" ) {
        $run_elapsed = "-";
    }
    else {
        $run_elapsed = $run_end - $run_start;
        $run_elapsed = convert_to_duration($run_elapsed);
    }

    # if no start/end time, set it to "-"
    # convert time to human-readable
    if ( $run_start == "" ) {
        $run_start = "-";
    }
    else {
        $run_start = convert_to_timestamp($run_start);
    }
    if ( $run_end == "" ) {
        $run_end = "-";
    }
    else {
        $run_end = convert_to_timestamp($run_end);
    }

    # try to come up with the link to the log directory
    if ( $output_dir == "" ) {
        $output_txt = "-";
    }
    else {
        $pos = strpos($output_dir,$archive_host);
        if ( $pos === false ) {
            # log directory is NOT on the archive host, don't try to hyperlink it
            $output_txt = $output_dir;
        }
        else {
            # attempt to hyperlink the output log
            $output_txt = "<a href=\"http:/$output_dir\">$output_dir</a>";
        }
    }

    # alternate row colors
    if ( ($i % 2) == 0 ) {
        echo "    <tr class=\"tr1\">\n";
    }
    else {
        echo "    <tr class=\"tr2\">\n";
    }
    echo "        <td><a href=\"show_test_runs_v1.php?runID=$run_id\">$run_id</a></td>\n";
    echo "        <td><a href=\"show_test_runs_v1.php?runID=$run_id\">$run_status</a></td>\n";
    echo "        <td>$run_name</td>\n";
    echo "        <td>$run_start</td>\n";
    echo "        <td>$run_end</td>\n";
    echo "        <td>$run_elapsed</td>\n";
    echo "        <td>$tc_count</td>\n";
    echo "        <td>$tc_pass</td>\n";
    echo "        <td>$tc_fail</td>\n";
    echo "        <td>$tc_skip</td>\n";
    echo "        <td>$tc_indeterminate</td>\n";
    echo "        <td>$tc_remaining</td>\n";
    echo "        <td>$output_txt</td>\n";
    echo "    </tr>\n";

    $i++;
}
echo "</table></p>";

mysql_close($conn);
?>

</body>
</html>
