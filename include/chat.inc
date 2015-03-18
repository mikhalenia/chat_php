<?php
require_once dirname(__FILE__).'/configurator.inc';
require_once dirname(__FILE__).'/logger.inc';
require_once dirname(__FILE__).'/auth.inc';
require_once dirname(__FILE__).'/terminal.inc';
require_once dirname(__FILE__).'/server.inc';
function getIp()
{
	exec("/sbin/route -n",$m);
	$aDRoute=preg_grep("/^0\\.0\\.0\\.0/m",$m);
	$sIface=preg_split("/\\pZ+/u",array_shift($aDRoute));
	$sIface=array_pop($sIface);
	unset($m,$aDRoute);
	exec("/sbin/ifconfig ".$sIface,$m);
	preg_match_all("/inet addr:([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)/",implode(" ",$m),$m);
	return $m[1][0];
}
/**
 * @param string $encoding
 * @param int $index
 * @param int $iTimeout
 * @param bool $bUseTZ
 * @return DBProvider
 */
function &getDbHandle($encoding='utf8',$iTimeout=60,$bUseTZ=true)
{
	global $config;
	include_once(dirname(__FILE__).'/DBProvider.inc');
	if(PHP_SAPI==='cli')
		DBProvider::$reconnect=true;
	if(isset($config['myDB']))
	{
		$db=$config['myDB'];
		$dsn='mysql:dbname='.$db['db'].';host='.(($db['host']==='localhost')?'127.0.0.1':$db['host']);
		if(isset($db['port']))
			$dsn.=';port='.$db['port'];
		else
			$dsn.=';port=3306';
		/**
		 * TZ setup instructions:
		 * http://dev.mysql.com/doc/refman/5.6/en/time-zone-support.html
		 */
		$TZ="Europe/Amsterdam";
		$aInit=array("NAMES ".($encoding?$encoding:'cp1251'));
		if($iTimeout>0)
			$aInit[]="SESSION wait_timeout=".$iTimeout;
		if($bUseTZ)
			$aInit[]="SESSION time_zone=\"{$TZ}\"";
		
		
		$sInit="SET ".implode(", ",$aInit);
		$aOptions=array(
			PDO::ATTR_AUTOCOMMIT => true,
			PDO::ATTR_CASE => PDO::CASE_NATURAL,
			PDO::ATTR_ERRMODE => PDO::ERRMODE_WARNING,
			PDO::MYSQL_ATTR_INIT_COMMAND => $sInit
		);
		$oProfiler=null;
		if(isset($_ENV["SQLPROFILE"]) || getenv("SQLPROFILE"))
			$oProfiler=dirname(__FILE__)."/../log/sql.log";
		$dbh=DBProvider::getInstance(
			$encoding,$dsn,
			$db['user'],$db['password'],
			$aOptions,$oProfiler
		);
		return $dbh;
	}
	$xFail=null;
	return $xFail;
}
?>