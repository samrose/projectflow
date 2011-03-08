<?php
// $Id: timeline.views.inc,v 1.1.2.2.2.1 2007/05/06 14:14:18 jmiccolis Exp $

/**
 * @file
 * Views API hook implementations for the timeline module.
 */

//////////////////////////////////////////////////////////////////////////////
// VIEWS API HOOKS

/**
 * Implementation of hook_views_style_plugins(). Adds view types to views UI interface.
 */
function _timeline_views_style_plugins() {
  return array(
    'timeline_vertical' => array(
      'name'         => t('Timeline Vertical'),
      'theme'        => 'views_view_timeline_vertical',
      'needs_fields' => true,
      //'validate'     => 'views_ui_plugin_validate_list',
    ),
    'timeline_horizontal' => array(
      'name'         => t('Timeline Horizontal'),
      'theme'        => 'views_view_timeline_horizontal',
      'needs_fields' => true,
      //'validate'     => 'views_ui_plugin_validate_list',
    ),
  );
}

/**
 * Implementation of hook_views_default_views().
 */
function _timeline_views_default_views() {
  $views = array();

  // Add timeline_nodes view
  $view = new stdClass();
  $view->name = 'timeline_nodes';
  $view->description = 'example timeline of nodes; accepts YYYY/MM args';
  $view->access = array (0 => '1', 1 => '2');
  $view->view_args_php = '';
  $view->page = TRUE;
  $view->page_title = 'Timeline of Content';
  $view->page_header = '';
  $view->page_header_format = '1';
  $view->page_footer = 'Drag timeline forward or backward to see more content.';
  $view->page_footer_format = '1';
  $view->page_empty = '';
  $view->page_empty_format = '1';
  $view->page_type = 'timeline_horizontal';
  $view->url = 'timeline/nodes';
  $view->use_pager = FALSE;
  $view->nodes_per_page = '50';
  $view->block = TRUE;
  $view->block_title = 'Timeline of Content';
  $view->block_header = '';
  $view->block_header_format = '1';
  $view->block_footer = '';
  $view->block_footer_format = '1';
  $view->block_empty = '';
  $view->block_empty_format = '1';
  $view->block_type = 'timeline_horizontal';
  $view->nodes_per_block = '50';
  $view->block_more = '0';
  $view->block_use_page_header = FALSE;
  $view->block_use_page_footer = TRUE;
  $view->block_use_page_empty = FALSE;
  $view->sort = array (
    array (
      'tablename' => 'node',
      'field' => 'created',
      'sortorder' => 'ASC',
      'options' => '',
    ),
  );
  $view->argument = array (
    array (
      'type' => 'year',
      'argdefault' => '2',
      'title' => '',
      'options' => '',
      'wildcard' => '',
      'wildcard_substitution' => '',
    ),
    array (
      'type' => 'month',
      'argdefault' => '2',
      'title' => '',
      'options' => '',
      'wildcard' => '',
      'wildcard_substitution' => '',
    ),
  );
  $view->field = array (
    array (
      'tablename' => 'node',
      'field' => 'created',
      'label' => 'timeline_start',
      'handler' => 'views_handler_field_date_small',
    ),
  );
  $view->filter = array (
    array (
      'tablename' => 'node',
      'field' => 'status',
      'operator' => '=',
      'options' => '',
      'value' => '1',
    ),
  );
  $view->exposed_filter = array ();
  $view->requires = array(node);
  $views[$view->name] = $view;

  // Add timeline_events view
  $view = new stdClass();
  $view->name = 'timeline_events';
  $view->description = 'example timeline of events; accepts YYYY/MM/DD args';
  $view->access = array (0 => '1', 1 => '2');
  $view->view_args_php = '';
  $view->page = TRUE;
  $view->page_title = 'Timeline of Events';
  $view->page_header = '';
  $view->page_header_format = '1';
  $view->page_footer = 'Drag timeline forward or backward to see more events.';
  $view->page_footer_format = '1';
  $view->page_empty = '';
  $view->page_empty_format = '1';
  $view->page_type = 'timeline_horizontal';
  $view->url = 'timeline/events';
  $view->use_pager = FALSE;
  $view->nodes_per_page = '50';
  $view->block = TRUE;
  $view->block_title = 'Timeline of Events';
  $view->block_header = '';
  $view->block_header_format = '1';
  $view->block_footer = '';
  $view->block_footer_format = '1';
  $view->block_empty = '';
  $view->block_empty_format = '1';
  $view->block_type = 'timeline_horizontal';
  $view->nodes_per_block = '50';
  $view->block_more = '0';
  $view->block_use_page_header = FALSE;
  $view->block_use_page_footer = TRUE;
  $view->block_use_page_empty = FALSE;
  $view->sort = array (
    array (
      'tablename' => 'event',
      'field' => 'event_start',
      'sortorder' => 'ASC',
      'options' => '',
    ),
  );
  $view->argument = array (
    array (
      'type' => 'event_year',
      'argdefault' => '2',
      'title' => '',
      'options' => 'event.event_start',
      'wildcard' => '',
      'wildcard_substitution' => '',
    ),
    array (
      'type' => 'event_month',
      'argdefault' => '2',
      'title' => '',
      'options' => 'event.event_start',
      'wildcard' => '',
      'wildcard_substitution' => '',
    ),
    array (
      'type' => 'event_day',
      'argdefault' => '2',
      'title' => '',
      'options' => 'event.event_start',
      'wildcard' => '',
      'wildcard_substitution' => '',
    ),
  );
  $view->field = array (
    array (
      'tablename' => 'event',
      'field' => 'event_start',
      'label' => '',
      'handler' => 'views_handler_field_date_small',
    ),
  );
  $view->filter = array (
    array (
      'tablename' => 'node',
      'field' => 'status',
      'operator' => '=',
      'options' => '',
      'value' => '1',
    ),
    array (
      'tablename' => 'event',
      'field' => 'event_start',
      'operator' => '>',
      'options' => '',
      'value' => '1970-08-02 11:01:18',
    ),
  );
  $view->exposed_filter = array();
  $view->requires = array(event, node);
  $views[$view->name] = $view;

  return $views;
}
