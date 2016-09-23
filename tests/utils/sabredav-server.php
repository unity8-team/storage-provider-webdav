<?php

// settings
date_default_timezone_set('UTC');
$publicDir = '.';

// Files we need
require_once 'Sabre/autoload.php';

class DummyAuth extends \Sabre\DAV\Auth\Backend\AbstractBasic {
    protected function validateUserPass($username, $password) {
        return true;
    }
}

// Make sure there is a directory in your current directory named 'public'. We will be exposing that directory to WebDAV
$rootNode = new \Sabre\DAV\FS\Directory($publicDir);

// The rootNode needs to be passed to the server object.
$server = new \Sabre\DAV\Server($rootNode);

$server->addPlugin(new \Sabre\DAV\Auth\Plugin(new DummyAuth(), "realm"));
$server->addPlugin(new \Sabre\DAV\Browser\GuessContentType());

// And off we go!
$server->exec();
