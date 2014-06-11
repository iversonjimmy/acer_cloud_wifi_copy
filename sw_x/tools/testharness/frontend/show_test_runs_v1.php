<html>
<head>
    <title>GVM Test Run Details</title>
    <link rel="stylesheet" type="text/css" href="style.css">
</head>
<body>

<?php

# get common values and functions
require("testharness_functions.php");

# function for printing test results in a table
function print_test_results($result)
{
    # global variables
    global $output_dir;
    global $archive_host;
    global $replace_string;
    global $pass_txt_style;
    global $fail_txt_style;
    global $style_end;
    global $spacer5;

    # print header of table
    echo "<table border=1>\n";
    echo "<tr>\n";
    echo "    <th>Test Suite</th>\n";
    echo "    <th>Test Case</th>\n";
    echo "    <th>Status</th>\n";
    echo "    <th>Log Dir</th>\n";
    echo "    <th>Elapsed Time</th>\n";
    echo "    <th>Start Time</th>\n";
    echo "    <th>End Time</th>\n";
    echo "</tr>\n";

    # count number of rows
    $num_rows = mysql_numrows($result);
    $i = 0;

    # print each result
    while ( $i < $num_rows ) {
        # get the individual values that are returned
        $ts_name = mysql_result($result,$i,"test_suite_name");
        $tc_name = mysql_result($result,$i,"test_case_name");
        $status = mysql_result($result,$i,"status");
        $epoch_start_time = mysql_result($result,$i,"start_time");
        $epoch_end_time = mysql_result($result,$i,"end_time");
        $run_order = mysql_result($result,$i,"run_order");
        $sub_name = mysql_result($result,$i,"sub_name");
        $sub_run_order = mysql_result($result,$i,"sub_run_order");

        # if no start/end time, set it to "-"
        # convert time to human-readable
        if ( $epoch_start_time == "" ) {
            $start_time = "-";
        }
        else {
            $start_time = convert_to_timestamp($epoch_start_time);
        }
        if ( $epoch_end_time == "" ) {
            $end_time = "-";
        }
        else {
            $end_time = convert_to_timestamp($epoch_end_time);
        }
        # calculate an elapsed time
        if ( $epoch_start_time == "" || $epoch_end_time == "" ) {
            $elapsed_time = "-";
        }
        else {
            $elapsed_secs = $epoch_end_time - $epoch_start_time;
            $elapsed_time = convert_to_duration($elapsed_secs);
        }

        # if it is a sub-test, use the sub-name and prefix the test name for visual separation
        if ( $sub_run_order > 0 ) {
            $tc_name = $spacer5 . $sub_name;
        }

        # add color to the status
        if ( $status == "PASS" ) {
            $status = $pass_txt_style . $status . $style_end;
        }
        elseif ( $status == "FAIL" ) {
            $status = $fail_txt_style . $status . $style_end;
        }

        # give a log dir hyperlink if it is a primary test
        if ( $sub_run_order == 0 || $sub_run_order == "" ) {
            # it's a primary test
            if ( $output_dir == "" ) {
                # no output_dir; can't do anything with this
                $log_dir_txt = "-";
            }
            else {
                # see if we can create a hyper-link for it
                $pos = strpos($output_dir,$archive_host);
                if ( $pos === false ) {
                    # log directory is NOT on the archive host, don't try to hyperlink it
                    $log_dir_txt = "$output_dir/$ts_name/$tc_name";
                }
                else {
                    # try and create a hyperlink to the filestore location
                    $log_dir_txt = "<a href=\"http:/$output_dir/$ts_name/$tc_name\">log dir</a>";
                }
            }
        }
        else {
            # it's a sub-test; no individual log
            $log_dir_txt = "-";
        }

        # alternate row color
        if ( ($i % 2) == 0 ) {
            echo "    <tr class=\"tr1\">\n";
        }
        else {
            echo "    <tr class=\"tr2\">\n";
        }

        # print the data
        echo "        <td>$ts_name</td>\n";
        echo "        <td>$tc_name</td>\n";
        echo "        <td>$status</td>\n";
        echo "        <td>$log_dir_txt</td>\n";
        echo "        <td>$elapsed_time</td>\n";
        echo "        <td>$start_time</td>\n";
        echo "        <td>$end_time</td>\n";
        echo "    </tr>\n";

        $i++;
    }

    echo "</table></p>";
}

# header
require("site_header.php");

# connect to the database
require("connect_to_db_v1.php");

$runID = $_GET['runID'];
#$runID = 31;

echo "\n<h2>Run details for run #$runID</h2>\n";

$query = "SELECT * FROM test_run WHERE id=$runID";
$result = mysql_query($query);
$num_rows = mysql_numrows($result);
if ( $num_rows != 1 ) {
    echo "<h2>ERROR: Unexpectedly found $num_rows entries for requested run #$runId; Results will be unpredictable</h2>\n";
}

# get summary info for the run
$run_status = mysql_result($result,0,"status");
$run_start = mysql_result($result,0,"start_time");
$run_end = mysql_result($result,0,"end_time");
$tc_count = mysql_result($result,0,"tc_count");
$tc_pass = mysql_result($result,0,"tc_pass");
$tc_fail = mysql_result($result,0,"tc_fail");
$tc_skip = mysql_result($result,0,"tc_skip");
$tc_indeterminate = mysql_result($result,0,"tc_indeterminate");
$output_dir = mysql_result($result,0,"output_dir");

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

# Print summary information
echo "<p>\n";
echo "<table border=0 cellpadding=1>\n";
echo "<tr>\n";
echo "    <td>$spacer3<b>Run Status</b>: $run_status</td>\n";
echo "    <td>$spacer3<b>Total Tests</b>: $tc_count</td>\n";
echo "    <td>$spacer3<b>Pass</b>: $tc_pass</td>\n";
echo "    <td>$spacer3<b>Fail</b>: $tc_fail</td>\n";
echo "    <td>$spacer3<b>Skip</b>: $tc_skip</td>\n";
echo "    <td>$spacer3<b>Indeterminate</b>: $tc_indeterminate</td>\n";
echo "</tr>\n";
echo "</table>\n";

# Print second row of summary information
echo "<table border=0 cellpadding=1>\n";
echo "<tr>\n";
echo "    <td>$spacer3<b>Start Time</b>: $run_start</td>\n";
echo "    <td>$spacer3<b>End Time</b>: $run_end</td>\n";
echo "    <td>$spacer3<b>Elapsed</b>: $run_elapsed</td>\n";
echo "</tr>\n";
echo "</table>\n";
echo "</p>\n";

# See if there are non-pass/failure tests to print up-front
$query = "SELECT ts.name AS test_suite_name,tc.name AS test_case_name,tci.status,tci.start_time,tci.end_time,tci.run_order,tci.sub_name,tci.sub_run_order FROM test_case_instance AS tci LEFT JOIN test_suite AS ts ON tci.test_suite_id=ts.id LEFT JOIN test_case AS tc ON tci.test_case_id=tc.id WHERE tci.status!=\"PASS\" AND tci.test_run_id=$runID ORDER BY run_order,sub_run_order";
$result = mysql_query($query);

$num_rows = mysql_numrows($result);
if ( $num_rows > 0 ) {
    echo "<h3>Failure Summary</h3>\n";
    print_test_results($result); 
}

# print all test results
$query = "SELECT ts.name AS test_suite_name,tc.name AS test_case_name,tci.status,tci.start_time,tci.end_time,tci.run_order,tci.sub_name,tci.sub_run_order FROM test_case_instance AS tci LEFT JOIN test_suite AS ts ON tci.test_suite_id=ts.id LEFT JOIN test_case AS tc ON tci.test_case_id=tc.id WHERE tci.test_run_id=$runID ORDER BY run_order,sub_run_order";
$result = mysql_query($query);

$num_rows = mysql_numrows($result);
if ( $num_rows > 0 ) {
    echo "<h3>All Tests</h3>\n";
    print_test_results($result); 
}
else {
    echo "<h3>No Test Results Found</h3>\n";
}

# close DB connection
mysql_close($conn);
?>

</body>
</html>
