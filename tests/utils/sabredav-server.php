<?php

// settings
date_default_timezone_set('UTC');
$publicDir = getcwd();

// The SabreDAV autoloader in Vivid is broken, so add one that just loads
// arbitrary files from /usr/share/php.
spl_autoload_register(function($class_name) {
    $file = "/usr/share/php/" . str_replace("\\", "/", $class_name) . ".php";
    if (file_exists($file)) {
        require_once $file;
    }
});

class MyFile extends \Sabre\DAV\FS\File {
    public function getETag() {
        $stat = stat($this->path);
        return '"' . $stat['dev'] . '.' . $stat['ino'] . '.' . $stat['mtime'] . '"';
    }
}

class MyDirectory extends \Sabre\DAV\FS\Directory {
    public function getChild($name) {
        $path = $this->path . '/' . $name;

        if (!file_exists($path)) throw new \Sabre\DAV\Exception\NotFound('File with name ' . $path . ' could not be located');

        if (is_dir($path)) {
            return new MyDirectory($path);
        } else {
            return new MyFile($path);
        }

    }
}

class DummyAuth extends \Sabre\DAV\Auth\Backend\AbstractBasic {
    protected function validateUserPass($username, $password) {
        return true;
    }
}

// Make sure there is a directory in your current directory named 'public'. We will be exposing that directory to WebDAV
$rootNode = new MyDirectory($publicDir);

// The rootNode needs to be passed to the server object.
$server = new \Sabre\DAV\Server($rootNode);

$server->addPlugin(new \Sabre\DAV\Auth\Plugin(new DummyAuth(), "realm"));
$server->addPlugin(new \Sabre\DAV\Browser\GuessContentType());

// And off we go!
$server->exec();
