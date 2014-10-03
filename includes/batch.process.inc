<?php

/**
 * @file
 * Common functions related to the islandora_batch framework.
 */

/**
 * Attempt to reparse a string into a more easily usable format.
 *
 * @param string $date_string_in
 *   A date we will pass to strtotime() to try to get a timestamp.
 *
 * @return string
 *   A string of the format YYYY-MM-DD.
 */
function uofm_newspaper_batch_fix_date($date_string_in) {
  $timestamp = strtotime((string) $date_string_in);
  $date_string_out = date('Y-m-d', $timestamp);
  return $date_string_out;
}