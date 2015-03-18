<?php
abstract class ChatLogger
{
	public static function mkpath($path,$mode = 0777)
	{
		if(is_link($path)) $path=readlink($path);
		return is_dir($path) || (self::mkpath(dirname($path),$mode) && self::_mkdir_($path,$mode));
	}
	private static function _mkdir_($path,$mode = 0777)
	{
		$old=umask(0);
		$res=@mkdir($path, $mode);
		umask($old);
		return $res;
	}
	public function log($iMsgCode,$sAddMsg='')
	{
		if(!$this->bLog)
			return false;
		$sMessage=isset(self::$aMessages[$iMsgCode])?self::$aMessages[$iMsgCode]:'';
		$sJSON=json_encode(array(
			$this->pid,
			posix_getpid(),
			$this->ip,
			date('c',time()),
			$iMsgCode,
			implode(' ',array($sMessage,$sAddMsg))
		)).PHP_EOL;
		return self::writeFile($this->sFileLog,$sJSON);
	}
	public static function writeFile($sFile,$sJSON,$bFlush=false)
	{
		if(!self::mkpath(dirname($sFile)))
			return false;
		$umask=umask(0);
		file_put_contents($sFile,$sJSON,($bFlush?LOCK_EX:FILE_APPEND|LOCK_EX));
		if((fileperms($sFile) & 0666)!=0666 && fileowner($sFile)==posix_getuid())
			@chmod($sFile,is_dir($sFile)?0777:0666);
        umask($umask);
		return true;
	}
}
?>