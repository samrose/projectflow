<?php
/* $Id: date_api_ical.inc,v 1.21.2.12 2008/04/17 12:06:38 karens Exp $ */
/**
 * @file
 * Parse iCal data.
 *
 * This file must be included when these functions are needed.
 */
/**
 * Return an array of iCalendar information from an iCalendar file.
 *
 *   No timezone adjustment is performed in the import since the timezone
 *   conversion needed will vary depending on whether the value is being
 *   imported into the database (when it needs to be converted to UTC), is being
 *   viewed on a site that has user-configurable timezones (when it needs to be
 *   converted to the user's timezone), if it needs to be converted to the
 *   site timezone, or if it is a date without a timezone which should not have
 *   any timezone conversion applied.
 *
 *   Properties that have dates and times are converted to sub-arrays like:
 *      'datetime'   => date in YYYY-MM-DD HH:MM format, not timezone adjusted
 *      'all_day'    => whether this is an all-day event
 *      'tz'         => the timezone of the date, could be blank for absolute
 *                      times that should get no timezone conversion.
 *
 *   Exception dates can have muliple values and are returned as arrays
 *   like the above for each exception date.
 *
 *   Most other properties are returned as PROPERTY => VALUE.
 *
 *   Each item in the VCALENDAR will return an array like:
 *   [0] => Array (
 *     [TYPE] => VEVENT
 *     [UID] => 104
 *     [SUMMARY] => An example event
 *     [URL] => http://example.com/node/1
 *     [DTSTART] => Array (
 *       [datetime] => 1997-09-07 09:00:00
 *       [all_day] => 0
 *       [tz] => US/Eastern
 *     )
 *     [DTEND] => Array (
 *       [datetime] => 1997-09-07 11:00:00
 *       [all_day] => 0
 *       [tz] => US/Eastern
 *     )
 *     [RRULE] => Array (
 *       [FREQ] => Array (
 *         [0] => MONTHLY
 *       )
 *       [BYDAY] => Array (
 *         [0] => 1SU
 *         [1] => -1SU
 *       )
 *     )
 *     [EXDATE] => Array (
 *       [0] = Array (
 *         [datetime] => 1997-09-21 09:00:00
 *         [all_day] => 0
 *         [tz] => US/Eastern
 *       )
 *       [1] = Array (
 *         [datetime] => 1997-10-05 09:00:00
 *         [all_day] => 0
 *         [tz] => US/Eastern
 *       )
 *     )
 *   )
 *
 * @param $filename
 *   Location (local or remote) of a valid iCalendar file
 * @return array
 *   An array with all the elements from the ical
 * @todo
 *   figure out how to handle this if subgroups are nested,
 *   like a VALARM nested inside a VEVENT.
 */
function date_ical_import($filename) {
  $items = array();

  // Fetch the iCal data. If file is a URL, use drupal_http_request. fopen
  // isn't always configured to allow network connections.
  if (substr($filename, 0, 4) == 'http') {
    // Fetch the ical data from the specified network location
    $icaldatafetch = drupal_http_request($filename);
    // Check the return result
    if ($icaldatafetch->error) {
      drupal_set_message('Request Error: '. $icaldatafetch->error, 'error');
      return false;
    }
    // Break the return result into one array entry per lines
    $icaldatafolded = explode("\n", $icaldatafetch->data);
  }
  else {
    $icaldatafolded = @file($filename, FILE_IGNORE_NEW_LINES);
    if ($icaldatafolded === FALSE) {
      drupal_set_message('Failed to open file: '. $filename, 'error');
      return false;
    }
  }
  return date_ical_parse($icaldatafolded);
}

/**
 * Return an array of iCalendar information from an iCalendar file.
 *
 * As date_ical_import() but different param.
 *
 * @param $icaldatafolded
 *   an array of lines from an ical feed.
 * @return array
 *   An array with all the elements from the ical.
 */
function date_ical_parse($icaldatafolded = array()) {
  // Verify this is iCal data
  if (trim($icaldatafolded[0]) != 'BEGIN:VCALENDAR') {
    drupal_set_message('Invalid calendar file:'. $filename, 'error');
    return false;
  }


  // "unfold" wrapped lines
  $icaldata = array();
  foreach ($icaldatafolded as $line) {
    $out = array();
    // See if this looks like the beginning of a new property or value.
    // If not, it is a continuation of the previous line.
    // The regex is to ensure that wrapped QUOTED-PRINTABLE data
    // is kept intact.
    if (!preg_match('/([A-Z]+)[:;](.*)/', $line, $out)) {
      $line = array_pop($icaldata) . ($line);
    }
    $icaldata[] = $line;
  }
  unset($icaldatafolded);

  // Parse the iCal information
  $parents = array();
  $subgroups = array();
  foreach ($icaldata as $line) {
    $line = trim($line);
    $vcal .= $line ."\n";
    // Deal with begin/end tags separately
    if (preg_match('/(BEGIN|END):V(\S+)/', $line, $matches)) {
      $closure = $matches[1];
      $type = 'V'. $matches[2];
      if ($closure == 'BEGIN') {
        array_push($parents, $type);
        array_push($subgroups, array());
      }
      else if ($closure == 'END') {
        end($subgroups);
        $subgroup =& $subgroups[key($subgroups)];
        switch ($type) {
          case 'VCALENDAR':
            if (prev($subgroups) == false) {
              $items[] = array_pop($subgroups);
            }
            else {
              $parent[array_pop($parents)][] = array_pop($subgroups);
            }
            break;
          // Add the timezones in with their index their TZID
          case 'VTIMEZONE':
            $subgroup = end($subgroups);
            $id = $subgroup['TZID'];
            unset($subgroup['TZID']);

            // Append this subgroup onto the one above it
            prev($subgroups);
            $parent =& $subgroups[key($subgroups)];

            $parent[$type][$id] = $subgroup;

            array_pop($subgroups);
            array_pop($parents);
            break;
          // Do some fun stuff with durations and all_day events
          // and then append to parent
          case 'VEVENT':
          case 'VALARM':
          case 'VTODO':
          case 'VJOURNAL':
          case 'VVENUE':
          case 'VFREEBUSY':
          default:
            // Can't be sure whether DTSTART is before or after DURATION,
            // so parse DURATION at the end.
            if (isset($subgroup['DURATION'])) {
              date_ical_parse_duration($subgroup);
            }
            // Check for all-day events setting the 'all_day' property under
            // the component instead of DTSTART/DTEND subcomponents
            if ((isset($subgroup['DTSTART']) && !isset($subgroup['DTEND'])) ||
            ($subgroup['DTSTART']['all_day'] && $subgroup['DTEND']['all_day'])) {
              $subgroup['all_day'] = true;
              if (!isset($subgroup['DTEND'])) {
                $subgroup['DTEND'] = $subgroup['DTSTART'];
              }
              unset($subgroup['DTSTART']['all_day']);
              unset($subgroup['DTEND']['all_day']);
            }
          // Add this element to the parent as an array under the
          // component name
          default:
            prev($subgroups);
            $parent =& $subgroups[key($subgroups)];

            $parent[$type][] = $subgroup;

            array_pop($subgroups);
            array_pop($parents);
            break;
        }
      }
    }
    // Handle all other possibilities
    else {
      // Grab current subgroup
      end($subgroups);
      $subgroup =& $subgroups[key($subgroups)];

      // Split up the line into nice pieces for PROPERTYNAME,
      // PROPERTYATTRIBUTES, and PROPERTYVALUE
      preg_match('/([^;:]+)(?:;([^:]*))?:(.+)/', $line, $matches);
      $name = strtoupper(trim($matches[1]));
      $field = $matches[2];
      $data = $matches[3];

      $parse_result = '';
      switch ($name) {
        // Keep blank lines out of the results.
        case '':
          break;

          // Lots of properties have date values that must be parsed out.
        case 'CREATED':
        case 'LAST-MODIFIED':
        case 'DTSTART':
        case 'DTEND':
        case 'DTSTAMP':
        case 'RDATE':
        case 'TRIGGER':
        case 'FREEBUSY':
        case 'DUE':
        case 'COMPLETED':
          $parse_result = date_ical_parse_date($field, $data);
          break;

        case 'EXDATE':
          $parse_result = date_ical_parse_exceptions($field, $data);
          break;

        case 'DURATION':
          // Can't be sure whether DTSTART is before or after DURATION in
          // the VEVENT, so store the data and parse it at the end.
          $subgroup['DURATION'] = array('DATA' => $data);
          break;

        case 'RRULE':
        case 'EXRULE':
          $parse_result = date_ical_parse_rrule($field, $data);
          break;

        case 'SUMMARY':
        case 'DESCRIPTION':
        case 'LOCATION':
          $parse_result = date_ical_parse_text($field, $data);
          break;

          // For all other properties, just store the property and the value.
          // This can be expanded on in the future if other properties should
          // be given special treatment.
        default:
          $parse_result = $data;
          break;
      }

      // Store the result of our parsing
      $subgroup[$name] = $parse_result;
    }
  }
  return $items;
}

/**
 * Parse a ical date element.
 *
 * Possible formats to parse include:
 *   PROPERTY:YYYYMMDD[T][HH][MM][SS][Z]
 *   PROPERTY;VALUE=DATE:YYYYMMDD[T][HH][MM][SS][Z]
 *   PROPERTY;VALUE=DATE-TIME:YYYYMMDD[T][HH][MM][SS][Z]
 *   PROPERTY;TZID=XXXXXXXX;VALUE=DATE:YYYYMMDD[T][HH][MM][SS]
 *   PROPERTY;TZID=XXXXXXXX:YYYYMMDD[T][HH][MM][SS]
 *
 *   The property and the colon before the date are removed in the import
 *   process above and we are left with $field and $data.
 *
 *  @param $field
 *    The text before the colon and the date, i.e.
 *    ';VALUE=DATE:', ';VALUE=DATE-TIME:', ';TZID='
 *  @param $data
 *    The date itself, after the colon, in the format YYYYMMDD[T][HH][MM][SS][Z]
 *    'Z', if supplied, means the date is in UTC.
 *
 *  @return array
 *   $items array, consisting of:
 *      'datetime'   => date in YYYY-MM-DD HH:MM format, not timezone adjusted
 *      'all_day'    => whether this is an all-day event with no time
 *      'tz'         => the timezone of the date, could be blank if the ical
 *                      has no timezone; the ical specs say no timezone
 *                      conversion should be done if no timezone info is
 *                      supplied
 *  @todo
 *   Another option for dates is the format PROPERTY;VALUE=PERIOD:XXXX. The period
 *   may include a duration, or a date and a duration, or two dates, so would
 *   have to be split into parts and run through date_ical_parse_date() and
 *   date_ical_parse_duration(). This is not commonly used, so ignored for now.
 *   It will take more work to figure how to support that.
 */
function date_ical_parse_date($field, $data) {
  // Turn the properties into a nice indexed array of
  // array(PROPERTYNAME => PROPERTYVALUE);
  $field_parts = preg_split('/[;:]/', $field);
  $properties = array();
  foreach ($field_parts as $part) {
    if (strpos($part, '=') !== false) {
      $tmp = explode('=', $part);
      $properties[$tmp[0]] = $tmp[1];
    }
  }

  // Make this a little more whitespace independent
  $data = trim($data);

  // Record if a time has been found
  $has_time = false;

  // If a format is specified, parse it according to that format
  if (isset($properties['VALUE'])) {
    switch ($properties['VALUE']) {
      case 'DATE':
        preg_match (DATE_REGEX_ICAL_DATE, $data, $regs);
        $datetime = date_pad($regs[1]) .'-'. date_pad($regs[2]) .'-'. date_pad($regs[3]); // Date
        break;
      case 'DATE-TIME':
        preg_match (DATE_REGEX_ICAL_DATETIME, $data, $regs);
        $datetime = date_pad($regs[1]) .'-'. date_pad($regs[2]) .'-'. date_pad($regs[3]); // Date
        $datetime .= ' '. date_pad($regs[4]) .':'. date_pad($regs[5]) .':'. date_pad($regs[6]); // Time
        $has_time = true;
        break;
    }
  }
  // If no format is specified, attempt a loose match
  else {
    preg_match (DATE_REGEX_LOOSE, $data, $regs);
    $datetime = date_pad($regs[1]) .'-'. date_pad($regs[2]) .'-'. date_pad($regs[3]); // Date
    if (isset($regs[4])) {
      $has_time = true;
      $datetime .= ' '.date_pad($regs[5]) .':'. date_pad($regs[6]) .':'. date_pad($regs[7]); // Time
    }
  }

  // Use timezone if explicitly declared
  if (isset($properties['TZID'])) {
    $tz = $properties['TZID'];
    // Fix commonly used alternatives like US-Eastern which should be US/Eastern.
    $tz = str_replace('-', '/', $tz);
  }
  // If declared as UTC with terminating 'Z', use that timezone
  else if (strpos($data, 'Z') !== false) {
    $tz = 'UTC';
  }
  // Otherwise this date is floating...
  else {
    $tz = '';
  }

  $items['datetime'] = $datetime;
  $items['all_day'] = $has_time ? false : true;
  $items['tz'] = $tz;
  return $items;
}

/**
 * Parse an ical repeat rule.
 *
 * @return array
 *   Array in the form of PROPERTY => array(VALUES)
 *   PROPERTIES include FREQ, INTERVAL, COUNT, BYDAY, BYMONTH, BYYEAR, UNTIL
 */
function date_ical_parse_rrule($field, $data) {
  $data = str_replace('RRULE:', '', $data);
  $items = array('DATA' => $data);
  $rrule = explode(';', $data);
  foreach ($rrule as $key => $value) {
    $param = explode('=', $value);
    if ($param[0] == 'UNTIL') {
      $values = date_ical_parse_date('', $param[1]);
    }
    else {
      $values = explode(',', $param[1]);
    }
    // Different treatment for items that can have multiple values and those that cannot.
    if (in_array($param[0], array('FREQ', 'INTERVAL', 'COUNT', 'WKST'))) {
      $items[$param[0]] = $param[1];
    }
    else {
      $items[$param[0]] = $values;
    }
  }
  return $items;
}

/**
 * Parse exception dates (can be multiple values).
 *
 * @return array
 *   an array of date value arrays.
 */
function date_ical_parse_exceptions($field, $data) {
  $data = str_replace('EXDATE:', '', $data);
  $items = array('DATA' => $data);
  $ex_dates = explode(',', $data);
  foreach ($ex_dates as $ex_date) {
    $items[] = date_ical_parse_date('', $ex_date);
  }
 return $items;
}

/**
 * Parse the duration of the event.
 * Example:
 *  DURATION:PT1H30M
 *  DURATION:P1Y2M
 *
 *  @param $subgroup
 *   array of other values in the vevent so we can check for DTSTART
 */
function date_ical_parse_duration(&$subgroup) {
  $items = $subgroup['DURATION'];
  $data  = $items['DATA'];
  preg_match('/^P(\d{1,4}[Y])?(\d{1,2}[M])?(\d{1,2}[W])?(\d{1,2}[D])?([T]{0,1})?(\d{1,2}[H])?(\d{1,2}[M])?(\d{1,2}[S])?/', $data, $duration);
  $items['year'] = isset($duration[1]) ? str_replace('Y', '', $duration[1]) : '';
  $items['month'] = isset($duration[2]) ?str_replace('M', '', $duration[2]) : '';
  $items['week'] = isset($duration[3]) ?str_replace('W', '', $duration[3]) : '';
  $items['day'] = isset($duration[4]) ?str_replace('D', '', $duration[4]) : '';
  $items['hour'] = isset($duration[6]) ?str_replace('H', '', $duration[6]) : '';
  $items['minute'] = isset($duration[7]) ?str_replace('M', '', $duration[7]) : '';
  $items['second'] = isset($duration[8]) ?str_replace('S', '', $duration[8]) : '';
  $start_date = array_key_exists('DTSTART', $subgroup) ? $subgroup['DTSTART']['datetime'] : date_format(date_now(), DATE_FORMAT_ISO);
  $timezone = array_key_exists('DTSTART', $subgroup) ? $subgroup['DTSTART']['tz'] : variable_get('date_default_timezone_name', NULL);
  if (empty($timezone)) {
    $timezone = 'UTC';
  }
  $date = date_make_date($start_date, $timezone);
  $date2 = drupal_clone($date);
  foreach ($items as $item => $count) {
    if ($count > 0) {
      date_modify($date2, '+'. $count .' '. $item);
    }
  }
  $format = isset($subgroup['DTSTART']['type']) && $subgroup['DTSTART']['type'] == 'DATE' ? 'Y-m-d' : 'Y-m-d H:i:s';
  $subgroup['DTEND'] = array(
    'datetime' => date_format($date2, DATE_FORMAT_DATETIME),
    'all_day' => isset($subgroup['DTSTART']['all_day']) ? $subgroup['DTSTART']['all_day'] : 0,
    'tz' => $timezone,
    );
  $items['DURATION'] = date_format($date2, 'U') - date_format($date, 'U');
  $subgroup['DURATION'] = $items;
}

/**
 * Parse and clean up ical text elements.
 */
function date_ical_parse_text($field, $data) {
  if (strstr($field, 'QUOTED-PRINTABLE')) {
    $data = quoted_printable_decode($data);
  }
  // Strip line breaks within element
  $data = str_replace(array("\r\n ", "\n ", "\r "), '', $data);
  // Put in line breaks where encoded
  $data = str_replace(array("\\n", "\\N"), "\n", $data);
  // Remove other escaping
  $data = stripslashes($data);
  return $data;
}

/**
 * Return a date object for the ical date, adjusted to its local timezone.
 *
 *  @param $ical_date
 *    an array of ical date information created in the ical import.
 *  @param $to_tz
 *    the timezone to convert the date's value to.
 *  @return object
 *    a timezone-adjusted date object
 */
function date_ical_date($ical_date, $to_tz = FALSE) {
  // If the ical date has no timezone, must assume it is stateless
  // so treat it as a local date.
  if (empty($ical_date['tz'])) {
    $from_tz = date_default_timezone_name();
  }
  else {
    $from_tz = $ical_date['tz'];
  }
  $date = date_make_date($ical_date['datetime'], $from_tz);
  if ($to_tz && $ical_date['tz'] != '' && $to_tz != $ical_date['tz']) {
    $date = date_timezone_set($date, timezone_open($to_tz));
  }
  return $date;
}

/**
 * Turn an array of events into a valid iCalendar file
 *
 * @param $events
 *   An array of events where each event is an array keyed on the uid:
 *    'start'         => start date object,
 *    'end'           => end date object,
 *                         optional, omit for all day event.
 *    'summary'       => Title of event (Text)
 *    'description'   => Description of event (Text)
 *    'location'      => Location of event (Text)
 *    'uid'           => ID of the event for use by calendaring program.
 *                         Recommend the url of the node
 *    'url'           => URL of event information
 *
 * @param $calname
 *   Name of the calendar.  Will use site name if none is specified.
 *
 * @return
 *   Text of a date_icalendar file.
 *
 * @todo
 *   add folding and more ical elements
 */
function date_ical_export($events, $calname = NULL) {
  $output .= "BEGIN:VCALENDAR\nVERSION:2.0\n";
  $output .= "METHOD:PUBLISH\n";
  $output .= 'X-WR-CALNAME:'. date_ical_escape_text($calname ? $calname : variable_get('site_name', '')) ."\n";
  $output .= "PRODID:-//Drupal iCal API//EN\n";
  foreach ($events as $uid => $event) {
    // Skip any items with empty dates.
    if (!empty($event['start'])) {
      $output .= "BEGIN:VEVENT\n";
      $output .= "DTSTAMP;TZID=". date_default_timezone_name() .";VALUE=DATE-TIME:". date_format(date_now(), DATE_FORMAT_ICAL) ."\n";
      $timezone = timezone_name_get(date_timezone_get($event['start']));
      if (!empty($timezone)) {
        $timezone = "TZID=$timezone;";
      }
      else {
        $timezone = '';
      }
      if ($event['start'] && $event['end']) {
        $output .= "DTSTART;". $timezone ."VALUE=DATE-TIME:". date_format($event['start'], DATE_FORMAT_ICAL) ."\n";
        $output .= "DTEND;". $timezone ."VALUE=DATE-TIME:". date_format($event['end'], DATE_FORMAT_ICAL) ."\n";
      }
      else {
        $output .= "DTSTART;". $timezone ."VALUE=DATE-TIME:". date_format($event['start'], DATE_FORMAT_ICAL) ."\n";
      }
      $output .= "UID:". ($event['uid'] ? $event['uid'] : $uid) ."\n";
      if ($event['url']) {
        $output .= "URL;VALUE=URI:". $event['url'] ."\n";
      }
      if ($event['location']) {
        $output .= "LOCATION:". date_ical_escape_text($event['location']) ."\n";
      }
      $output .= "SUMMARY:". date_ical_escape_text($event['summary']) ."\n";
      if ($event['description']) {
        $output .= "DESCRIPTION:". date_ical_escape_text($event['description']) ."\n";
      }
      $output .= "END:VEVENT\n";
    }
  }
  $output .= "END:VCALENDAR\n";
  return $output;
}

/**
 * Escape #text elements for safe iCal use
 *
 * @param $text
 *   Text to escape
 *
 * @return
 *   Escaped text
 *
 */
function date_ical_escape_text($text) {
  //$text = strip_tags($text);
  $text = str_replace('"', '\"', $text);
  $text = str_replace("\\", "\\\\", $text);
  $text = str_replace(",", "\,", $text);
  $text = str_replace(":", "\:", $text);
  $text = str_replace(";", "\;", $text);
  $text = str_replace("\n", "\n ", $text);
  return $text;
}

/**
 * Build an iCal RULE from $form_values.
 *
 * @param $form_values
 *   an array constructed like the one created by date_ical_parse_rrule()
 *
 *     [RRULE] => Array (
 *       [FREQ] => Array (
 *         [0] => MONTHLY
 *       )
 *       [BYDAY] => Array (
 *         [0] => 1SU
 *         [1] => -1SU
 *       )
 *       [UNTIL] => Array (
 *         [datetime] => 1997-21-31 09:00:00
 *         [all_day] => 0
 *         [tz] => US/Eastern
 *       )
 *     )
 *     [EXDATE] => Array (
 *       [0] = Array (
 *         [datetime] => 1997-09-21 09:00:00
 *         [all_day] => 0
 *         [tz] => US/Eastern
 *       )
 *       [1] = Array (
 *         [datetime] => 1997-10-05 09:00:00
 *         [all_day] => 0
 *         [tz] => US/Eastern
 *       )
 *     )
 *
 */
function date_api_ical_build_rrule($form_values) {
  //grab the RRULE data and put them into iCal RRULE format
  $RRULE = 'RRULE:FREQ='. $form_values['FREQ'];
  $RRULE .= ';INTERVAL='. $form_values['INTERVAL'];

  // Unset the empty 'All' values.
  unset($form_values['BYDAY']['']);
  unset($form_values['BYMONTH']['']);
  unset($form_values['BYMONTHDAY']['']);

  if ($form_values['BYDAY']) {
    $RRULE .= ';BYDAY='. implode(",", $form_values['BYDAY']);
  }
  if ($form_values['BYMONTH']) {
    $RRULE .= ';BYMONTH='. implode(",", $form_values['BYMONTH']);
  }
  if ($form_values['BYMONTHDAY']) {
    $RRULE .= ';BYMONTHDAY='. implode(",", $form_values['BYMONTHDAY']);
  }
  if ($form_values['UNTIL']['datetime']) {
    $RRULE .= ';UNTIL='. date_convert($form_values['UNTIL']['datetime'], DATE_DATETIME, DATE_ICAL);
  }

  // iCal rules presume the week starts on Monday unless otherwise specified,
  // so we'll specify it.
  if (isset($form_values['WKST'])) {
    $RRULE .= ';WKST='. $form_values['WKST'];
  }
  else {
    $RRULE .= ';WKST='. date_repeat_dow2day(variable_get('date_first_day', 1));
  }

  // Exceptions dates go last, on their own line.
  if (is_array($form_values['EXDATE'])) {
    $ex_dates = array();
    foreach ($form_values['EXDATE'] as $value) {
      $ex_date = date_convert($value['datetime'], DATE_DATETIME, DATE_ICAL);
      if (!empty($ex_date)) {
        $ex_dates[] = $ex_date;
      }
    }
    if (!empty($ex_dates)) {
      sort($ex_dates);
      $RRULE .= chr(13) . chr(10) .'EXDATE:'. implode(',', $ex_dates);
    }
  }
  elseif (!empty($form_values['EXDATE'])) {
    $RRULE .= chr(13) . chr(10) .'EXDATE:'. $form_values['EXDATE'];
  }
  return $RRULE;
}