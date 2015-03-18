#!/usr/bin/php
<?php
include_once(dirname(__FILE__)."/include/chat.inc");
switch($argv[1])
{
	case 'start':
		foreach($config['chatListen'] as $aListen)
		{
			$oServer=new ChatServer($aListen['host'],$aListen['port'],true);
			if($oServer->isRunning())
				printf("Server %s:%s is running.\n",$aListen['host'],$aListen['port']);
			elseif($oServer->start())
				printf("Server %s:%s has started.\n",$aListen['host'],$aListen['port']);
            else
                printf("Could not start the server %s:%s.\n",$aListen['host'],$aListen['port']);
		}
		break;
	case 'stop':
		foreach($config['chatListen'] as $aListen)
		{
			$oServer=new ChatServer($aListen['host'],$aListen['port'],true);
            if($oServer->isRunning()&&$oServer->stop())
                printf("Server %s:%s has stopped.\n",$aListen['host'],$aListen['port']);
        }
		break;
	default:
		echo "Usage: chat {start|stop}\n";
		break;
}
printf("OK\n");
?>