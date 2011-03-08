<?php
// $Id: print_mail.inc,v 1.8.2.2.2.1 2008/10/16 12:30:57 jcnventura Exp $

/**
 * @file
 * Displays and processes the mail send form.
 *
 * This file is included by the print_mail module and includes the
 * mail form display, validation and submit hooks.
 */

require_once(drupal_get_path('module', 'print') .'/print.pages.inc');

/**
 * Menu callback for the send by e-mail form.
 *
 * @ingroup forms
 */
function print_mail_form($form_state) {
  global $user;

  $print_mail_hourly_threshold = variable_get('print_mail_hourly_threshold', PRINT_MAIL_HOURLY_THRESHOLD);

  if (!flood_is_allowed('print_mail', $print_mail_hourly_threshold)) {
    $form['flood'] = array(
      '#type' => 'markup',
      '#value' => '<p>'. t('You cannot send more than %number messages per hour. Please try again later.', array('%number' => $print_mail_hourly_threshold)) .'</p>',
    );
    return $form;
  }

  $print_mail_teaser_default = variable_get('print_mail_teaser_default', PRINT_MAIL_TEASER_DEFAULT_DEFAULT);
  $print_mail_teaser_choice = variable_get('print_mail_teaser_choice', PRINT_MAIL_TEASER_CHOICE_DEFAULT);
  $form = array();

  // Remove the printmail/ prefix
  $path = explode('/', $_GET['q']);
  unset($path[0]);
  $path = implode('/', $path);
  if (is_numeric($path)) {
    $path = 'node/'. $path;
  }
  $cid = isset($_GET['comment']) ? (int)$_GET['comment'] : NULL;
  $form['path'] = array('#type' => 'value', '#value' => $path);
  $form['cid'] =  array('#type' => 'value', '#value' => $cid);

  $form['fld_from_addr'] = array(
    '#type' => 'textfield',
    '#title' => t('Your e-mail'),
    '#size' => 62,
  );
  $form['fld_from_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Your name'),
    '#size' => 62,
  );
  $form['txt_to_addrs'] = array(
    '#type' => 'textarea',
    '#title' => t('Send to'),
    '#rows' => 3,
    '#resizable' => FALSE,
    '#description' => t('Enter multiple addresses separated by commas and/or different lines.'),
  );
  $form['fld_subject'] = array(
    '#type' => 'textfield',
    '#title' => t('Subject'),
    '#size' => 62,
  );
  $form['txt_message'] = array(
    '#type' => 'textarea',
    '#title' => t('Your message'),
    '#rows' => 6,
  );

  if ($print_mail_teaser_choice) {
    $form['chk_teaser'] = array(
      '#type' => 'checkbox',
      '#title' => t('Send only the teaser'),
      '#default_value' => $print_mail_teaser_default,
    );
  }
  else {
    $form['chk_teaser'] = array('#type' => 'value', '#value' => $print_mail_teaser_default);
  }

  $form['btn_submit'] = array(
    '#name' => 'submit',
    '#type' => 'submit',
    '#value' => t('Send e-mail'),
  );
  $form['btn_clear'] = array(
    '#type' => 'markup',
    '#value' => '<input type="reset" name="clear" value="'. t('Clear form') .'" class="form-submit" />',
  );
  $form['btn_cancel'] = array(
    '#name' => 'cancel',
    '#type' => 'submit',
    '#value' => t('Cancel'),
  );

  if ($user->uid != 0) {
    $user_name = $user->name;
    $form['fld_from_addr']['#default_value'] = $user->mail;
    $form['fld_from_addr']['#disabled'] = TRUE;
    $form['fld_from_addr']['#value'] = $user->mail;
    $form['fld_from_name']['#default_value'] = $user->name;
  }
  else {
    $user_name = t('Someone');
  }
  $site_name = variable_get('site_name', t('an interesting site'));
  $form['fld_subject']['#default_value'] = t('!user has sent you a message from !site',
                                              array('!user' => $user_name, '!site' => $site_name));

  return $form;
}

/**
 * Theme function for the send by-email form submission.
 *
 * Adds a class to the form labels. This class is used to place the label on
 * the left of the input fields.
 *
 * @ingroup forms
 */
function theme_print_mail_form($form) {
  drupal_add_css(drupal_get_path('module', 'print') .'/css/printlinks.css');
  $content = '';
  foreach (element_children($form) as $key) {
    $tmp = drupal_render($form[$key]);
    switch ($key) {
      case 'fld_from_addr':
      case 'fld_from_name':
      case 'txt_to_addrs':
      case 'fld_subject':
        $tmp = str_replace('<label', '<label class ="printmail-label"', $tmp);
        break;
    }
    $content .= $tmp;
  }
  return $content;
}

/**
 * Validate the send by-email form submission.
 *
 * @ingroup forms
 */
function print_mail_form_validate($form_id, $form_values, $form) {
  if (array_key_exists('cancel', $form['#post'])) {
    return;
  }
  $from_addr = trim($form_values['fld_from_addr']);
  $test = user_validate_mail($from_addr);
  if ($test) {
    form_set_error('fld_from_addr', $test);
  }

  // All new-lines are replaced by commas
  $to_addrs = str_replace(array("\r\n", "\n", "\r"), ',', $form_values['txt_to_addrs']);
  // Create an array from the string
  $to_array = explode(',', $to_addrs);
  // Verify each element of the array
  foreach ($to_array as $key => $address) {
    $address = trim($address);
    if (preg_match('/(.*?) <(.*)>/s', $address, $matches)) {
      // Address is of the type User Name <user@domain.tld>
      $test = user_validate_mail($matches[2]);
      $to_array[$key] = trim($matches[1]) .' <'. $matches[2] .'>';
    }
    else {
      // Address must be user@domain.tld
      $test = user_validate_mail($address);
    }
    if ($test) {
      form_set_error('txt_to_addrs', $test);
    }
  }

  // In all fields, prevent insertion of custom headers
  foreach ($form_values as $key => $string) {
    if ( (substr($key, 0, 4) == 'fld_') && ((strpos($string, "\n") !== FALSE) || (strpos($string, "\r") !== FALSE)) ) {
      form_set_error($key, 'Found invalid character');
    }
  }

  $subject = trim($form_values['fld_subject']);
  if (empty($subject)) {
    form_set_error('fld_subject', t('You must enter a subject.'));
  }
  $message = trim($form_values['txt_message']);
  if (empty($message)) {
    form_set_error('txt_message', t('You must enter a message.'));
  }

  form_set_value($form['fld_from_addr'], $from_addr);
  form_set_value($form['fld_from_name'], trim($form_values['fld_from_name']));
  // Re-create the string from the re-organized array
  form_set_value($form['txt_to_addrs'], implode(', ', $to_array));
  form_set_value($form['fld_subject'], $subject);
  form_set_value($form['txt_message'], $message);
}

/**
 * Process the send by-email form submission.
 *
 * @ingroup forms
 */
function print_mail_form_submit($form_id, $form_values) {
  if (!array_key_exists('cancel', $form_values)) {
  if (!empty($form_values['fld_from_name'])) {
    $from = '"'. $form_values['fld_from_name'] .'" <'. $form_values['fld_from_addr'] .'>';
  }
  else {
    $from = $form_values['fld_from_addr'];
  }
  $to = $form_values['txt_to_addrs'];
  $params = array();
  $params['subject'] = $form_values['fld_subject'];
  $params['message'] = $form_values['txt_message'];
  $params['path'] = $form_values['path'];
  $params['cid'] = isset($form_values['cid']) ? $form_values['cid'] : '';
  $params['teaser'] = $form_values['chk_teaser'];

  print_mail_mail('sendpage', $message, $params);

  $ret = drupal_mail('print_mail_sendpage', $to, $message['subject'], $message['body'], $from, $message['headers']);
  if ($ret) {
    flood_register_event('print_mail');
    watchdog('print_mail', t('%name [%from] sent %page to [%to]', array('%name' => $form_values['fld_from_name'], '%from' => $form_values['fld_from_addr'], '%page' => $form_values['path'], '%to' => $to)));
    $site_name = variable_get('site_name', t('us'));
    drupal_set_message(t('Thank you for spreading the word about !site.', array('!site' => $site_name)));
  }
  }
  return preg_replace('!^book/export/html/!', 'node/', $form_values['path']);
}

/**
 * Implementation of hook_mail().
 */
function print_mail_mail($key, &$message, $params) {
  switch ($key) {
    case 'sendpage':
      $sender_message = t('Message from sender:') .'<br /><br /><em>'. nl2br($params['message']) .'</em>';

      $print = print_controller($params['path'], $params['cid'], $params['teaser'], $sender_message);
      if ($print === FALSE) {
        return;
      }

      // Img elements must be set to absolute
//      $pattern = '!<(img\s[^>]*?)>!is';
//      $print['content'] = preg_replace_callback($pattern, '_print_rewrite_urls', $print['content']);
//      $print['logo'] = preg_replace_callback($pattern, '_print_rewrite_urls', $print['logo']);
//      $print['footer_message'] = preg_replace_callback($pattern, '_print_rewrite_urls', $print['footer_message']);

      $node = $print['node'];
      ob_start();
      include_once(_print_get_template('mail', $print['type']));
      $html = ob_get_contents();
      ob_end_clean();

      $message['subject'] = $params['subject'];
      $message['body'] = $html;
      $message['headers']['Content-Type'] = 'text/html; charset=utf-8';
    break;
  }
}
