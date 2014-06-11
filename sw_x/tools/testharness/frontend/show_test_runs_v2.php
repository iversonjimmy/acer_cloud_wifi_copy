<html>
<head>
    <title>GVM Test Run Details</title>
    <link rel="stylesheet" type="text/css" href="style.css">
</head>
<body>

<?php

# get common values and functions
require("testharness_functions.php");

# print summary info
function print_summary_info($sql_result)
{
    # global vars
    global $spacer3;

    # get summary info for the run
    $run_name = mysql_result($sql_result,0,"name");
    $run_status = mysql_result($sql_result,0,"status");
    $run_start = mysql_result($sql_result,0,"start_time");
    $run_end = mysql_result($sql_result,0,"end_time");
    $tc_count = mysql_result($sql_result,0,"tc_count");
    $tc_pass = mysql_result($sql_result,0,"tc_pass");
    $tc_fail = mysql_result($sql_result,0,"tc_fail");
    $tc_expected_to_fail = mysql_result($sql_result,0,"tc_expected_to_fail");
    $tc_indeterminate = mysql_result($sql_result,0,"tc_indeterminate");
    $output_dir = mysql_result($sql_result,0,"output_dir");

    # add color to the status
    $run_status = colorize_status($run_status);

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
    echo "<b>Name</b>: $run_name\n";
    echo "<table border=0 cellpadding=1>\n";
    echo "<tr>\n";
    echo "    <td>$spacer3<b>Run Status</b>: $run_status</td>\n";
    echo "    <td>$spacer3<b>Total Tests</b>: $tc_count</td>\n";
    echo "    <td>$spacer3<b>Pass</b>: $tc_pass</td>\n";
    echo "    <td>$spacer3<b>Fail</b>: $tc_fail</td>\n";
    echo "    <td>$spacer3<b>Exp Fail</b>: $tc_expected_to_fail</td>\n";
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
}

# print table header
function print_table_header()
{
    echo "<table border=1>\n";
    echo "<tr>\n";
    echo "    <th>Test Suite</th>\n";
    echo "    <th>Test Case</th>\n";
    echo "    <th>Status</th>\n";
    echo "    <th>Logs</th>\n";
    echo "    <th>Elapsed Time</th>\n";
    echo "    <th>Start Time</th>\n";
    echo "    <th>End Time</th>\n";
    echo "</tr>\n";
}

# print table closer
function print_table_closer()
{
    echo "</table>\n";
    echo "</p>\n";
}

# return a string that colorizes a PASS/FAIL status
function colorize_status($status)
{
    global $pass_txt_style;
    global $fail_txt_style;
    global $indeterminate_txt_style;
    global $style_end;

    # add color to the status
    if ( $status == "PASS" || $status == "EXPECTED_TO_FAIL" ) {
        return $pass_txt_style . $status . $style_end;
    }
    elseif ( $status == "FAIL" ) {
        return $fail_txt_style . $status . $style_end;
    }
    elseif ( $status == "INDETERMINATE" ) {
        return $indeterminate_txt_style . $status . $style_end;
    }
    else {
        return $status;
    }
}

# return a string that dims text color
function dim_text($text)
{
    global $dim_txt_style;
    global $style_end;

    return $dim_txt_style . $text . $style_end;
}

# print a single test suite result
# return the name of the test suite
function print_test_suite_result($sql_result,$row)
{
    # global vars
    global $archive_host;
    global $spacer3;

    # get the values 
    $ts_name = mysql_result($sql_result,$row,"name");
    $epoch_start_time = mysql_result($sql_result,$row,"start_time");
    $epoch_end_time = mysql_result($sql_result,$row,"end_time");
    $ts_status = mysql_result($sql_result,$row,"status");
    $run_output_dir = mysql_result($sql_result,$row,"output_dir");

    # calculate times
    # if no start/end time, set it to "-"
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

    # colorize the status
    $ts_status = colorize_status($ts_status);

    # determine the summary log file and the log directory
    if ( $run_output_dir == "" ) {
        # no output_dir; can't do anything with this
        $ts_summary_log = "-";
        $ts_output_dir = "-";
    }
    else {
        # see if the output log is on the archive host - if it is then we can try to hyperlink the log and directory
        $pos = strpos($run_output_dir,$archive_host);
        if ( $pos === false ) {
            # don't bother trying to hyperlink
            $ts_summary_log = "$run_output_dir/${ts_name}.out";
            $ts_output_dir = "$run_output_dir/${ts_name}";
        }
        else {
            # log is on the archive host, modify the output_dir name to make it hyperlink-able
            $run_output_dir = str_replace("build@pcstore.ctbg.acer.com:/a/","",$run_output_dir);

            $ts_summary_log = "<a href=\"http://pcstore.ctbg.acer.com/$run_output_dir/${ts_name}.output\"/>summary_log</a>";
            $ts_output_dir = "<a href=\"http://pcstore.ctbg.acer.com/$run_output_dir/${ts_name}\"/>save_dir</a>";
        }
    }

    # print the test suite data
    echo "    <tr class=\"suite\">\n";
    echo "        <td>$ts_name</td>\n";
    echo "        <td>&nbsp</td>\n";
    echo "        <td>$ts_status</td>\n";
    echo "        <td>${ts_summary_log}${spacer3}${ts_output_dir}</td>\n";
    echo "        <td>$elapsed_time</td>\n";
    echo "        <td>$start_time</td>\n";
    echo "        <td>$end_time</td>\n";
    echo "    </tr>\n";

    return $ts_name;
}

# printing test results in a table
function print_test_case_results($sql_result)
{
    # count number of rows
    $num_rows = mysql_numrows($sql_result);
    $i = 0;

    # print each result
    while ( $i < $num_rows ) {
        # get the individual values that are returned
        $ts_name = mysql_result($sql_result,$i,"test_suite_name");
        $tc_name = mysql_result($sql_result,$i,"test_case_name");
        $status = mysql_result($sql_result,$i,"status");

        # dim the suite name
        $ts_name = dim_text($ts_name);

        # colorize the status
        $status = colorize_status($status);

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
        echo "        <td>&nbsp;</td>\n";
        echo "        <td>&nbsp;</td>\n";
        echo "        <td>&nbsp;</td>\n";
        echo "        <td>&nbsp;</td>\n";
        echo "    </tr>\n";

        $i++;
    }
}

#
# MAIN starts here
#

# header
require("site_header.php");

# connect to the database
require("connect_to_db_v2.php");

$runID = $_GET['runID'];
#$runID = 31;

echo "\n<h2>Run details for run #$runID</h2>\n";

# print summary information about the run
$query = "SELECT * FROM test_run WHERE id=$runID";
$test_run_result = mysql_query($query);
$num_test_run_rows = mysql_numrows($test_run_result);
if ( $num_test_run_rows != 1 ) {
    echo "<h2>ERROR: Unexpectedly found $num_rows entries for requested run #$runId; Results will be unpredictable</h2>\n";
}
else {
    print_summary_info($test_run_result);
}

# see if there are non-pass/failure tests to print up-front
$query = "SELECT test_suite.name,test_suite_instance.start_time,test_suite_instance.end_time,test_suite_instance.status,test_run.output_dir FROM test_suite_instance LEFT JOIN test_suite ON test_suite_instance.test_suite_id=test_suite.id LEFT JOIN test_run ON test_suite_instance.test_run_id=test_run.id WHERE test_run_id=$runID AND test_suite_instance.status!=\"PASS\" ORDER BY test_suite_instance.id";
$test_suite_result = mysql_query($query);
if ( !$test_suite_result ) {
    $num_test_suite_rows = 0;
}
else {
    $num_test_suite_rows = mysql_numrows($test_suite_result);
}

if ( $num_test_suite_rows > 0 ) {
    echo "<h3>Failure Summary</h3>\n";
    print_table_header();

    for ($i=0; $i < $num_test_suite_rows; $i++) {
        # test suite status
        $suite_name = print_test_suite_result($test_suite_result,$i);

        # print failed test cases
        $query = "SELECT ts.name AS test_suite_name, tc.name AS test_case_name, tci.status FROM test_case_instance AS tci LEFT JOIN test_suite_instance AS tsi ON tci.test_suite_instance_id=tsi.id LEFT JOIN test_suite AS ts ON tsi.test_suite_id=ts.id LEFT JOIN test_case AS tc ON tci.test_case_id=tc.id WHERE tci.test_run_id=$runID AND ts.name=\"$suite_name\" AND tci.status!=\"PASS\" ORDER BY tci.id";
        $test_case_result = mysql_query($query);
        $num_test_case_rows = mysql_numrows($test_case_result);

        if ( $num_test_case_rows > 0 ) {
            print_test_case_results($test_case_result);
        }
        # else no failed test cases
    }

    print_table_closer();
}
# else no failures to report

# print all test results
# this consists of printing the suite status, then the individual TC status for that suite
$query = "SELECT test_suite.name,test_suite_instance.start_time,test_suite_instance.end_time,test_suite_instance.status,test_run.output_dir FROM test_suite_instance LEFT JOIN test_suite ON test_suite_instance.test_suite_id=test_suite.id LEFT JOIN test_run ON test_suite_instance.test_run_id=test_run.id WHERE test_run_id=$runID ORDER BY test_suite_instance.id";
$test_suite_result = mysql_query($query);
$num_test_suite_rows = mysql_numrows($test_suite_result);
if ( $num_test_suite_rows == 0 ) {
    echo "<h3>No Test Results Found</h3>\n";
}
else {
    echo "<h3>All Tests</h3>\n";
    print_table_header();

    for ($i=0; $i < $num_test_suite_rows; $i++) {
        # test suite status
        $suite_name = print_test_suite_result($test_suite_result,$i);

        # test case results
        $query = "SELECT ts.name AS test_suite_name, tc.name AS test_case_name, tci.status FROM test_case_instance AS tci LEFT JOIN test_suite_instance AS tsi ON tci.test_suite_instance_id=tsi.id LEFT JOIN test_suite AS ts ON tsi.test_suite_id=ts.id LEFT JOIN test_case AS tc ON tci.test_case_id=tc.id WHERE tci.test_run_id=$runID AND ts.name=\"$suite_name\" ORDER BY tci.id";
        $test_case_result = mysql_query($query);
        $num_test_case_rows = mysql_numrows($test_case_result);

        if ( $num_test_case_rows > 0 ) {
            print_test_case_results($test_case_result);
        }
        # else no test case results found for this suite
    }

    print_table_closer();
}

# close DB connection
mysql_close($conn);
?>

</body>
</html>
