<?php

/**
 * @file
 * Example XML-RPC client for creating a Case Tracker case.
 *
 * Requires a valid user on the Drupal installation which has the
 * "create cases via XML-RPC" access permission set. For the most
 * flexibility, the XML-RPC client is responsible for building up
 * a Drupal $node array that is then passed to node_save().
 *
 * This particular example uses XML-RPC for PHP
 * from http://sourceforge.net/projects/phpxmlrpc/
 */

include_once('./xmlrpc-2.1/lib/xmlrpc.inc');

$client = new xmlrpc_client("/xmlrpc.php", "example.com", 80);

// encode our values for transport,
$username = php_xmlrpc_encode('valid drupal username');
$password = php_xmlrpc_encode('valid drupal password');
$node = php_xmlrpc_encode(array(
  'title'            => 'XML-RPC case posted from xmlrpc_example.php',
  'body'             => 'Delete this test case, submitted via XML-RPC.',
  'type'             => 'casetracker_basic_case',
  'uid'              => 1,
  'pid'              => 14,
  'status'           => 1,
  'case_priority_id' => 1,
  'case_status_id'   => 7,
  'case_type_id'     => 14,
));

// send out the request to the server.
$message = new xmlrpcmsg('casetracker.newCase', array($username, $password, $node));
$response = $client->send($message);

// success/error handling.
if (!$response->faultCode()) {
  print php_xmlrpc_decode($response->value()) . "\n";
}
else {
  print "An error occurred. Code: ". htmlspecialchars($response->faultCode()) .
        " Reason: '". htmlspecialchars($response->faultString()) ."'\n";
}
