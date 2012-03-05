function installCallback(message)
{
	alert(message);
}

function install()
{

	var dbHost     = $('#db_host');
	var dbUserName = $('#db_username');
	var dbPassword = $('#db_password');

	jQuery.post("carson_api.php",
		{
			action:		 	'install',
			db_host: 		dbHost.val(),
			db_username: 	dbUserName.val(),
			db_password: 	dbPassword.val()
		}, installCallback );
		
}

$(function() {

	var buttons =
		{	
			"Install": install
		};
			
	// Set the default name for the database.		
	$( "#db_host" ).val("localhost");	

	$( "#install_form" ).dialog({
			autoOpen: true,
			width:  500,
			height: 330,
			modal: false,
			resizable: false,
			position: 'center',
			draggable: false,
			closeOnEscape: false,
			beforeclose: function(){ return false },
			buttons: buttons
		});


	// Create the button to open the new project dialog.
	$("#install_button").button().click( install );
		
});
