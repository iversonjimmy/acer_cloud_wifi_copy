<?php

$spacer3 = "&nbsp;&nbsp;&nbsp;";
$spacer5 = "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;";
$pass_txt_style = "<span style=\"color:#006600\">";
$fail_txt_style = "<span style=\"color:#FF0033\">";
$dim_txt_style = "<span style=\"color:#666666\">";
$indeterminate_txt_style = "<span style=\"color:#FF0099\">";
$skip_txt_style = "<span style=\"color:black\">";
$style_end = "</span>";
$archive_host = "pcstore";
$replace_string = "pcstore.ctbg.acer.com/pc/test_outputs";


function convert_to_timestamp($epoch_time)
# given epoch_seconds, convert it to a human-readable timestamp
{
    # Note, to show date in UTC time, use gmdate() instead of date()
    #return gmdate('Y-m-d H:i:s T',$epoch_time);

    # return timestamp like "2010-11-10 14:16:23 UTC"
    return date('m/d/Y H:i:s T',$epoch_time);
}

function convert_to_duration($seconds)
# given a number of seconds, convert it to a human-readable duration
{
    $secs_in_min = 60;
    $secs_in_hour = $secs_in_min * 60;
    $secs_in_day = $secs_in_hour * 24;

    $days = (int)($seconds / $secs_in_day);
    $seconds = $seconds - ($days * $secs_in_day);
    $hours = (int)($seconds / $secs_in_hour);
    $seconds = $seconds - ($hours * $secs_in_hour);
    $minutes = (int)($seconds / $secs_in_min);
    $seconds = $seconds - ($minutes * $secs_in_min);

    # only print out days and hours if there are days and hours to print
    $return_txt = "";
    if ( $days > 0 ) {
        $return_txt = $return_txt . "${days}d ";
    }
    if ( $hours > 0 ) {
        $return_txt = $return_txt . "${hours}h ";
    }
    # always show minutes and seconds
    $return_txt = $return_txt . "${minutes}m ${seconds}s";

    return $return_txt;
}

?>
