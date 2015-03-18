<?php
error_reporting(E_ERROR);
if(PHP_SAPI!='cli')
{
	echo "Use this chat as CLI";
	exit(0);
}

$bIsDebug=(bool)array_search('debug',$argv);

if($bIsDebug)
{
	//Turn on display_errors
	ini_set('display_errors', true);
	ini_set('display_startup_errors', true);
	// Display ALL errors including notices
	error_reporting(65535);
}

include(dirname(__FILE__).'/config.inc');

?>