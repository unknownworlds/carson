<?php

require_once("carson.php");

if (!isset($_POST['action']))
{
	echo "An action must be specified";
	return;
}

$action = $_POST['action'];

if (!carson_installed())
{
	if ($action == 'install')
	{
		$dbHost     = $_POST['db_host'];
		$dbUserName = $_POST['db_username'];
		$dbPassword = $_POST['db_password'];
		carson_install($dbHost, $dbUserName, $dbPassword);
		echo "Installation complete";
		return;
	}
	else
	{
		echo "Carson is not installed";
		return;
	}
}

carson_connect();

if ($action == 'create_project')
{
	if (isset($_POST['name']) && isset($_POST['command']) && isset($_POST['trigger']))
	{
		$name    = $_POST['name'];
		$command = $_POST['command'];
		$trigger = $_POST['trigger'];
		$project = new Project();
		$project->name    = $name;
		$project->command = $command;
		$project->trigger = $trigger;
		carson_addProject($project);
	}
}
else if ($action == 'update_project')
{
	if (isset($_POST['projectId']) && isset($_POST['name']) && isset($_POST['command']) && isset($_POST['trigger']))
	{
		$id 	 = $_POST['projectId'];
		$name    = $_POST['name'];
		$command = $_POST['command'];
		$trigger = $_POST['trigger'];
		$project = new Project();
		$project->id	  = $id;
		$project->name    = $name;
		$project->command = $command;
		$project->trigger = $trigger;
		carson_updateProject($project);
	}
}
else if ($action == 'delete_project')
{
	if (isset($_POST['projectId']))
	{
		$id = $_POST['projectId'];
		carson_deleteProject($id);
	}
}
else if ($action == 'run_project')
{
	if (isset($_POST['projectId']))
	{
		$id = $_POST['projectId'];
		carson_buildProject($id);
	}
}
else if ($action == 'enable_project')
{
	if (isset($_POST['projectId']) && isset($_POST['enabled']) )
	{
		$id		 = $_POST['projectId'];
		$enabled = $_POST['enabled'] == "true";
		carson_enableProject($id, $enabled);
	}
}
else if ($action == 'get_results')
{
	if (isset($_POST['projectId']))
	{
		$id = $_POST['projectId'];

        $openedEvents = (isset($_POST['openedEvents'])) ? explode(',', $_POST['openedEvents']) : false;
		echo carson_getProjectLog($id, $openedEvents);
	}
}
else if ($action = 'get_projects')
{
	// Returns a list of the projects in JSON format.
	$projects = carson_getProjects();
	
	header("Content-type: application/json");
	
	$result = array(); 
	$result['project'] = array();
	
	foreach ($projects as $i => $project)
	{
	
		$time = date("l F j, Y h:i:s A", $project->time);
		$state = "";
		
		if ($project->state == "new")
		{
			$state = "Project has not been run yet";
		}
		else if ($project->state == "building")
		{
			$state = "Started on $time";
		}
		else if ($project->state == "pending")
		{
			$state = "Queued for run";
		}
		else if ($project->state == "failed")
		{
			$state = "Run failed at $time";
		}
		else if ($project->state == "succeeded")
		{
			$state = "Run succeeded on $time";
		}
			
		$result['project'][] = 
			array(
					'id' => $project->id,
					'name' => $project->name,
					'state' => $state,
					'status' => $project->state,
					'command' => $project->command,
					'trigger' => $project->trigger,
					'enabled' => $project->enabled,
				);

	}	
	
	echo json_encode($result);
	
}
else
{
	echo "Unrecognized action '$action'";
}

?>