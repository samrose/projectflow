<?php
// $Id: timeline.theme.inc,v 1.1.2.8.2.4 2008/03/16 23:15:14 jmiccolis Exp $

/**
 * @file
 * Default theming functions for the timeline module.
 */

//////////////////////////////////////////////////////////////////////////////
// VIEWS API HOOKS

/**
 * Displays a Drupal view using a vertical timeline widget.
 */
function theme_views_view_timeline_vertical($view, $nodes) {
  return theme('views_view_timeline', $view, $nodes, 'VERTICAL');
}

/**
 * Displays a Drupal view using a horizontal timeline widget.
 */
function theme_views_view_timeline_horizontal($view, $nodes) {
  return theme('views_view_timeline', $view, $nodes, 'HORIZONTAL');
}

/**
 * Displays a Drupal view using a timeline widget.
 */
function theme_views_view_timeline($view, $nodes, $orientation = 'HORIZONTAL') {
  $initial_focus = timeline_variable_get($view->vid, $view->build_type, 'focus', TIMELINE_INITIAL_DATE);

  if ($initial_focus == 'today') {
    $initial_date = time();
  }
  else {
    // Use the start/end date of the first or last node, respectively, for
    // the timeline. This assumes that the view is sorted in ascending order
    // based on the start date.

    if ($initial_focus == 'first') {
      $node = node_load(array('nid' => $nodes[0]->nid));

      // Assume the view's first field represents the start date.
      if (!empty($view->field[0]['field'])) {
        $field = $view->field[0]['field'];
        if (preg_match('/^(field_.*)_value$/', $field, $matches)) {
          $value = $node->$matches[1];
          $initial_date = $value[0]['value']; // CCK field
        }
        else {
          $initial_date = $node->$field; // ordinary field
        }
      }

      $initial_date = !empty($initial_date) ? $initial_date : $node->created;
    }
    else { // FIXME
      $initial_date = !empty($initial_date) ? $initial_date : time();
    }

    if (!is_numeric($initial_date))
      $initial_date = _timeline_strtotime($initial_date);
  }

  return theme('timeline', array(
    'id'           => $view->name,
    'view_name'    => $view->name,
    'view_args'    => $view->args,
    'orientation'  => strtoupper($orientation),
    'width'        => timeline_variable_get($view->vid, $view->build_type, 'width', TIMELINE_WIDTH),
    'height'       => timeline_variable_get($view->vid, $view->build_type, 'height', TIMELINE_HEIGHT),
    'initial_date' => timeline_format_initial_date($initial_date),
    'band1_unit'   => timeline_variable_get($view->vid, $view->build_type, 'band1_unit', 'day'),
    'band2_unit'   => timeline_variable_get($view->vid, $view->build_type, 'band2_unit', 'month'),
    'controls'     => timeline_variable_get($view->vid, $view->build_type, 'controls', FALSE),
  ));
}

//////////////////////////////////////////////////////////////////////////////
// DRUPAL API HOOKS

/**
 * Returns the HTML for initializing a timeline widget.
 *
 * @param $timeline an associative array with the following variables set:
 *
 *   id - the XHTML identifier of the timeline element
 *   width - the width of the timeline widget
 *   height - the height of the timeline widget
 *   class - CSS class of the timeline element (default 'timeline')
 *   align - left, right or center (default '')
 *   orientation - HORIZONTAL or VERTICAL
 *   initial_date - the initial focus date of the timeline
 *   timezone - the timezone represented by # of hours difference from GMT
 *   controls - whether to show the filtering controls
 *   view_name - the name of the Drupal view to render in the timeline
 *   view_args - an array of additional view arguments
 *
 * @return a string containing the timeline HTML.
 *
 */
function theme_timeline($timeline) {
  global $base_url;

  if (strpos(TIMELINE_WIDGET_URL, '://') === FALSE) { // relative URL
    drupal_add_js(TIMELINE_WIDGET_URL);
  }
  else { // absolute URL
    drupal_set_html_head('<script type="text/javascript" src="' . TIMELINE_WIDGET_URL . '"></script>');
  }

  drupal_add_css(drupal_get_path('module', 'timeline') . '/timeline.css');
  drupal_add_js(drupal_get_path('module', 'timeline') . '/timeline.js');

  $timeline = (object)$timeline;
  if (empty($timeline->class))        $timeline->class = 'timeline';
  if (empty($timeline->width))        $timeline->width = TIMELINE_WIDTH;
  if (empty($timeline->height))       $timeline->height = TIMELINE_HEIGHT;
  if (empty($timeline->align))        $timeline->align = '';
  if (empty($timeline->id))           $timeline->id = 'timeline';
  if (empty($timeline->orientation))  $timeline->orientation = TIMELINE_ORIENTATION;
  if (empty($timeline->initial_date)) $timeline->initial_date = timeline_format_initial_date(time());
  if (empty($timeline->controls))     $timeline->controls = FALSE;

  // FIXME: timezone fiddling due to bug in the SIMILE widget; see timeline_format_iso8601_date().
  //if (!isset($timeline->timezone)) $timeline->timezone = timeline_get_timezone() / 3600;
  if (!isset($timeline->timezone)) $timeline->timezone = 0;

  $style = array('width' => _timeline_to_dim($timeline->width), 'height' => _timeline_to_dim($timeline->height));
  switch (strtolower($timeline->align)) {
    case 'left':
    case 'right':
      $style['float']  = strtolower($timeline->align);
      break;
    case 'center':
      $style += array('margin-left' => 'auto', 'margin-right' => 'auto');
      break;
  }
  $style = implode('; ', array_map_assoc(create_function('$k, $v', 'return "$k: $v";'), $style));
  $tag = _timeline_content_tag('div', '', array('id' => $timeline->id, 'class' => $timeline->class, 'style' => $style));
  $output .= _timeline_content_tag('div', $tag, array('class' => $timeline->class . '-wrapper'));
  if ($timeline->controls) {
    $output .= _timeline_content_tag('div', '', array('class' => $timeline->class . '-controls', 'id' => $timeline->id . '-controls'));
  }

  $query = 'view=' . $timeline->view_name . (empty($timeline->view_args) ? '' : '&args=' . urlencode(serialize($timeline->view_args)));
  $url = url('timeline/' . TIMELINE_FEED_TYPE, $query, NULL, FALSE);
  $args = array($timeline->id , '%O', $timeline->initial_date, $timeline->timezone, array('%U1', '%U2'), $url, TIMELINE_FEED_TYPE);
  $script = 'createTimelineWidget(' . implode(', ', array_map('drupal_to_js', $args)) . ');';
  if ($timeline->controls) {
    $texts = implode(', ', array_map('drupal_to_js', array(t('Filter:'), t('Highlight:'), t('Clear All'))));
    $script = 'var timeline = ' . $script . " setupTimelineControls('{$timeline->id}-controls', timeline, $texts);";
  }
  $script = 'if (Drupal.jsEnabled) { $(document).ready(function() { ' . $script . ' }); }';
  $script = '<script type="text/javascript">' . $script . '</script>';
  $script = str_replace('"%O"', 'Timeline.' . $timeline->orientation, $script); // if only PHP had symbols...
  $script = str_replace('"%U1"', 'Timeline.DateTime.' . strtoupper($timeline->band1_unit), $script);
  $script = str_replace('"%U2"', 'Timeline.DateTime.' . strtoupper($timeline->band2_unit), $script);
  $output .= $script;

  return $output;
}

//////////////////////////////////////////////////////////////////////////////
// TIMELINE HELPERS

if (!function_exists('array_map_assoc')) {
  function array_map_assoc($callback, $array) {
    return array_map($callback, array_keys($array), array_values($array));
  }
}

function _timeline_to_dim($string) {
  return intval($string) . (strpos($string, '%') !== FALSE ? '%' : 'px');
}

function _timeline_tag_options($options) {
  return implode(' ', array_map_assoc(create_function('$k, $v', 'return $k . \'="\' . htmlspecialchars($v) . \'"\';'), $options));
}

function _timeline_content_tag($name, $content = '', $options = NULL) {
  return "<$name" . (!$options ? '' : ' ' . _timeline_tag_options($options)) . ">$content</$name>";
}

function _timeline_tag($name, $options = NULL, $open = FALSE) {
  return "<$name" . (!$options ? '' : ' ' . _timeline_tag_options($options)) . ($open ? '>' : ' />');
}
