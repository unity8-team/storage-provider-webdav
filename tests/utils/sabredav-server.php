<?php

/*

This example demonstrates a simple way to create your own virtual filesystems.
By extending the _File and Directory classes, you can easily create a tree
based on various datasources.

The most obvious example is the filesystem itself. A more complete and documented
example can be found in:

lib/Sabre/DAV/FS/Node.php
lib/Sabre/DAV/FS/Directory.php
lib/Sabre/DAV/FS/File.php

*/

// settings
date_default_timezone_set('UTC');
$publicDir = 'public';

// Files we need
require_once 'sabre21/Sabre/autoload.php';

class MyCollection extends Sabre\DAV\Collection {
    private $myPath;

    function __construct($myPath) {
        $this->myPath = $myPath;
    }

    function getChildren() {
        $children = [];
        // Loop through the directory, and create objects for each node
        foreach(scandir($this->myPath) as $node) {
            // Ignoring files staring with .
            if ($node[0]==='.') continue;
            $children[] = $this->getChild($node);
        }
        return $children;
    }

    function getChild($name) {
        $path = $this->myPath . '/' . $name;

        // We have to throw a NotFound exception if the file didn't exist
        if (!file_exists($this->myPath)) {
            throw new \Sabre\DAV\Exception\NotFound('The file with name: ' . $name . ' could not be found');
        }
        // Some added security
        if ($name[0]==='.') {
            throw new \Sabre\DAV\Exception\Forbidden('Access denied');
        }
        if (is_dir($path)) {
            return new \MyCollection($name);
        } else {
            return new \MyFile($path);
        }
    }

    function getName() {
        return basename($this->myPath);
    }
}

class MyFile extends \Sabre\DAV\File {
    private $myPath;

    function __construct($myPath) {
        $this->myPath = $myPath;
    }

    function getName() {
        return basename($this->myPath);
    }

    function get() {
        return fopen($this->myPath,'r');
    }

    function getSize() {
        return filesize($this->myPath);
    }
}

// Make sure there is a directory in your current directory named 'public'. We will be exposing that directory to WebDAV
$rootNode = new \MyCollection($publicDir);

$authBackend = new \Sabre\DAV\Auth\Backend\BasicCallBack(function($user, $pass) {
    return true;
});
$authPlugin = new \Sabre\DAV\Auth\Plugin($authBackend);

// The rootNode needs to be passed to the server object.
$server = new \Sabre\DAV\Server($rootNode);
$server->addPlugin($authPlugin);

// And off we go!
$server->exec();
