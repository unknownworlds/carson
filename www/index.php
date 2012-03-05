<?php

require_once("carson.php");

if (!carson_installed())
{
	$template = file_get_contents("templates/install.html");
}
else
{
	$template = file_get_contents("templates/index.html");
}

echo $template;

?>