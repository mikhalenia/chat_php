<?php
class DBStatement extends PDOStatement
{
	private $sLogFile;
	protected function __construct($sLogFile=null)
	{
		$this->sLogFile=$sLogFile;
	}
	public function execute($x=null)
	{
		$aParams=func_get_args();
		if(count($aParams)==1 && is_array($aParams[0]))
			$aParams=$aParams[0];
		if($this->sLogFile)
			$tm=microtime(true);
		if($aParams)
		{
			$aRealParams=array();
			foreach($aParams as $xItem)
				if(is_array($xItem))
				{
					$aItem=array_values($xItem);
					$aRealParams=array_merge($aRealParams,$aItem);
				}
				else
					$aRealParams[]=$xItem;
			$aParams=$aRealParams;
			$bResult=parent::execute($aParams);
		}
		else
			$bResult=parent::execute();
		if($this->sLogFile)
			$this->log(microtime(true)-$tm,$this->reconstruct($aParams));
		return $bResult;
	}
	protected function reconstruct($aParams)
	{
		if(!$aParams)
			return $this->queryString;
		$aChunk=explode("?",$this->queryString);
		$sQueryString="";
		$sLastPart=array_pop($aChunk);
		foreach($aChunk as $sChunk)
		{
			$s=array_shift($aParams);
			if(is_null($s))
				$s="NULL";
			else
				$s='"'.str_replace('"','\\"',strval($s)).'"';
			$sQueryString.=$sChunk.$s;
		}
		return $sQueryString.$sLastPart;
	}
	protected function log($tm,$s)
	{
		if($fp=fopen($this->sLogFile,"a"))
		{
			$s=preg_replace("/^\\s+/m","",$s);
			$bt=debug_backtrace();
			array_shift($bt);
			array_shift($bt);
			$a=array_shift($bt);
			$s=sprintf(
				"[%.4f] %s:%s[%s]: %s\n-------------------------------------------------------\n",
				$tm,$a["file"],$a["line"],$a["function"],$s
			);
			fwrite($fp,$s);
			fclose($fp);
			$u=umask(0);
			chmod($this->sLogFile,0666);
			umask($u);
		}
	}
}
class DBProvider extends PDO
{
	private static $instance;
	private $sLogFile;
	private $sInstanceKey;
	public static $reconnect=false;
	public function __construct($dsn,$user="",$password="",$options=array(),$sLogFile=null)
	{
		$this->sLogFile=$sLogFile;
		try
		{
			$options[PDO::ATTR_STATEMENT_CLASS]=array("DBStatement",array($sLogFile));
			$encoding=$options["custom_encoding"];
			unset($options["custom_encoding"]);
			parent::__construct($dsn,$user,$password,$options);
			$this->sInstanceKey=self::getInstanceKey($encoding,$dsn,$user,$password);
		}
		catch(PDOException $e){}
	}
	private function __clone() {}
	private static function getInstanceKey($encoding,$dsn,$user,$password)
	{
		return implode(":",array($encoding,$dsn,$user,$password));
	}
	public static function &getInstance($encoding,$dsn,$user="",$password="",$options=array(),$sLogFile=null)
	{
		$key=self::getInstanceKey($encoding,$dsn,$user,$password);
		if(is_null(self::$instance))
		{
			self::$instance=array();
			register_shutdown_function("CloseDBProvider");
		}
		if(isset(self::$instance[$key]))
		{
			if(!self::$reconnect)
				return self::$instance[$key];
			/**
			 * Verify connection
			 */
			$xErrMode=self::$instance[$key]->getAttribute(PDO::ATTR_ERRMODE);
			self::$instance[$key]->setAttribute(PDO::ATTR_ERRMODE,PDO::ERRMODE_SILENT);
			@self::$instance[$key]->query("SELECT 1");
			$aInfo=self::$instance[$key]->errorInfo();
			if($aInfo[0]==='00000')
			{
				self::$instance[$key]->setAttribute(PDO::ATTR_ERRMODE,$xErrMode);
				return self::$instance[$key];
			}
			else
				unset(self::$instance[$key]);
		}
		$options["custom_encoding"]=$encoding;
		self::$instance[$key]=new self($dsn,$user,$password,$options,$sLogFile);
		return self::$instance[$key];
	}
	public static function shutdown()
	{
		if(self::$instance)
			foreach(array_keys(self::$instance) as $k)
				self::$instance[$k]=null;
	}
	public function &query($sQueryString)
	{
		if($this->sLogFile)
		{
			$tm=microtime(true);
		}
		$oStatement=parent::query($sQueryString);
		if($this->sLogFile)
			$this->log(microtime(true)-$tm,$sQueryString);
		return $oStatement;
	}
	public function exec($sQueryString)
	{
		if($this->sLogFile)
		{
			$tm=microtime(true);
		}
		$iResult=parent::exec($sQueryString);
		if($this->sLogFile)
		{
			$sLogMsg=($iResult===false)?'FAIL: ':'SUCCESS: ';
			$this->log(microtime(true)-$tm,$sLogMsg.$sQueryString);
		}
		return $iResult;
	}
	public function quote($string,$paramtype=PDO::PARAM_STR)
	{
		if(is_array($string))
		{
			$x=array();
			foreach($string as $k=>$v)
				$x[$k]=$this->quote($v,$paramtype);
			return $x;
		}
		return parent::quote($string,$paramtype);
	}
	/**
	 * Escapes characters that are treated as special operators by the sphinx
	 * query language parser. Returns an escaped string.
	 *
	 * @param string $string is a string to escape.
	 * @return string an escaped string
	 */
	public function EscapeString($string)
	{
		static $from=array('\\','(',')','|','-','!','@','~','"','&','/','^','$','=');
		static $to=array('\\\\','\(','\)','\|','\-','\!','\@','\~','\"','\&','\/','\^','\$','\=');
		return str_replace($from,$to,$string);
	}
	public function marks($iCount)
	{
		return implode(",",array_fill(0,$iCount,'?'));
	}
	public function now()
	{
		static $iNow;
		if(is_null($iNow))
		{
			$iNow=$this->query("SELECT UNIX_TIMESTAMP(NOW())")->fetchColumn();
			$iNow=$iNow-$iNow%60+59;
		}
		return $iNow;
	}
	protected function log($tm,$s)
	{
		if($fp=fopen($this->sLogFile,"a"))
		{
			$s=preg_replace("/^\\s+/m","",$s);
			$bt=debug_backtrace();
			array_shift($bt);
			array_shift($bt);
			$a=array_shift($bt);
			$s=sprintf(
				"[%.4f] %s:%s[%s]: %s\n-------------------------------------------------------\n",
				$tm,$a["file"],$a["line"],$a["function"],$s
			);
			fwrite($fp,$s);
			fclose($fp);
			$u=umask(0);
			chmod($this->sLogFile,0666);
			umask($u);
		}
	}
}
function CloseDBProvider()
{
	DBProvider::shutdown();
}
?>