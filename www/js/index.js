var selectedProjectId = -1;
var projects = { };

function validateForm()
{

	var name    = $( "#name" );
	var command = $( "#command" );
	var trigger = $( "#trigger" );

	var allFields = $( [] ).add( name ).add( command ).add( trigger );
	var tips = $( ".validateTips" );

	function updateTips( t )
	{
		tips.text( t ).addClass( "ui-state-highlight" );
		setTimeout(function() {
			tips.removeClass( "ui-state-highlight", 1500 );
		}, 500 );
	}

	function checkSet( o, n ) {
		if ( o.val().length > 0) {
			return true;
		}
		else {
			o.addClass( "ui-state-error" );
			updateTips( n + " must be specified" );
			return false;
		}
	}

	var valid = true;
	allFields.removeClass( "ui-state-error" );
	valid = valid && checkSet( name, "name" );
	valid = valid && checkSet( command, "command" );

	return valid;

}

function setSelectedProject(id)
{

	var results = $( "#results" );

	if (id != -1)
	{
		if (id != window.selectedProjectId)
		{
			results.html("");
		}
		results.load("carson_api.php", { action: 'get_results', projectId: id });
		$("#project_" + id).addClass("ui-selected").siblings().removeClass("ui-selected");
		if (window.selectedProjectId != id)
		{
			results.show( "slide", {}, 200 );
		}
	}
	else
	{
		results.hide();
		$(".project").removeClass("ui-selected");
	}

	window.selectedProjectId = id;

}

function projectSelect(event)
{
	var id = $(event.currentTarget).find('.project_id').val();
	setSelectedProject(id);
}

function createProjectElement(project)
{

	var projectElement = $( templateRender("#project_template", project) );

	projectElement.click(function(event) {
			projectSelect(event);
		});

	// Setup the button handlers.

	var enableButton = projectElement.find(".project_enable_checkbox");
	enableButton.button({
         icons: {
                primary: "ui-icon-power",
            },
            text: false
		});
	enableButton.click(projectEnableButtonClick);
	enableButton.prop('checked', project.enabled == '0');
	enableButton.button( "refresh" );

	var runButton = projectElement.find(".project_run_button");
	runButton.button({
         icons: {
                primary: "ui-icon-play",
            },
            text: false
		});
	runButton.click(projectRunButtonClick);

	var editButton = projectElement.find(".project_edit_button");
	editButton.button({
         icons: {
                primary: "ui-icon-gear",
            },
            text: false
		});
	editButton.click(projectEditButtonClick);

	var deleteButton = projectElement.find(".project_delete_button");
	deleteButton.button({
         icons: {
                primary: "ui-icon-trash",
            },
            text: false
		});
	deleteButton.click(projectDeleteButtonClick);

	projectElement.find(".project_status").addClass("project_status_" + project.status );

	// return the DOM object instead of the selector.
	return projectElement.get(0);

}

/** Inserts the node into the parent after insert (or at the beginning
if insert is null */
function insertAfter(parent, insert, node)
{
	if (insert != null)
	{
		parent.insertBefore( node, insert.nextSibling  );
	}
	else
	{
		parent.insertBefore( node, parent.firstElementChild  );
	}
}

function updateProjectsCallback(data, textStatus, jqXHR)
{

	var oldProjects =  window.projects;
	var newProjects = data['project'];
	var lastElement = null;

	var i = 0;
	var j = 0;

	var projectsElement = $('#projects');
	var insert = null;

	while (i < newProjects.length && j < oldProjects.length)
	{

		var newProject = newProjects[i];
		var oldProject = oldProjects[j];

		if (newProject.id < oldProject.id)
		{
			// A project was added.
			var projectElement = createProjectElement(newProject);
			insertAfter( projectsElement.get(0), insert, projectElement);
			insert = projectElement;
			++i;
		}
		else if (newProject.id > oldProject.id)
		{
			// A project was removed.
			$("#project_" + oldProject.id).remove();
			if (window.selectedProjectId == oldProject.id)
			{
				setSelectedProject(-1);
			}
			++j;
		}
		else
		{
			// Update the element.
			var projectElement = $("#project_" + oldProject.id);

			if (newProject.name  != oldProject.name ||
			    newProject.state != oldProject.state)
			{
				projectElement.replaceWith( createProjectElement(newProject) );
				projectElement = $("#project_" + oldProject.id);
				if (oldProject.id == window.selectedProjectId)
				{
					setSelectedProject(oldProject.id);
				}
			}

			insert = projectElement.get(0);
			++i;
			++j;
		}

	}

	while (i < newProjects.length)
	{
		var newProject = newProjects[i];
		// Add the project.
		var projectElement = createProjectElement(newProject);
		insertAfter( projectsElement.get(0), insert, projectElement);
		insert = projectElement;
		++i;
	}

	while (j < oldProjects.length)
	{
		var oldProject = oldProjects[j];
		// Remove the project.
		$("#project_" + oldProject.id).remove();
		if (window.selectedProjectId == oldProject.id)
		{
			setSelectedProject(-1);
		}
		++j;
	}

	window.projects = newProjects;

}

/** Asynchronously updates the pane that displays from the server. */
function updateProjects()
{
	jQuery.post("carson_api.php", { action: 'get_projects' }, updateProjectsCallback, 'json');
}

function update()
{
	var results = $( "#results" );

    var openedEvents = [];
    $('#results pre:visible').each(function(){
        openedEvents.push($('#results .element pre').index(this)+1);
    })

	results.load("carson_api.php", { action: 'get_results', projectId: window.selectedProjectId, openedEvents: openedEvents.join() } );
}

function showErrorMessage(message)
{
	$("#error_message_text").html(message);
	var error = $("#error_message");
	error.show();
}

function projectActionCallback(message)
{
	updateProjects();
	if (message.length > 0)
	{
		showErrorMessage(message);
	}
}

function projectRunButtonClick(event)
{
	var id = $(event.target).parents('li').find('input').val();
	jQuery.post("carson_api.php", { action: 'run_project', projectId: id }, projectActionCallback );
	setSelectedProject(id);
	return false;
}

function projectEnableButtonClick(event)
{
	var id = $(event.target).parents('li').find('input').val();
	var enabled = event.target.checked ? 'false' : 'true';
	jQuery.post("carson_api.php", { action: 'enable_project', projectId: id, enabled: enabled}, projectActionCallback );
	setSelectedProject(id);
	return true;
}

function projectEditButtonClick(event)
{
	var id = $(event.target).parents('li').find('input').val();

	var name    = $( "#name" );
	var command = $( "#command" );
	var trigger = $( "#trigger" );

	var project = null;
	for (var i = 0; i < window.projects.length; ++i)
	{
		if (window.projects[i].id == id)
		{
			project = window.projects[i];
			break;
		}
	}
	if (project == null)
	{
		showErrorMessage("Project doesn't exist");
		return;
	}

	name.val( project.name );
	command.val( project.command );
	trigger.val( project.trigger );

	var allFields = $( [] ).add( name ).add( command ).add( trigger );
	var tips = $( ".validateTips" );

	function updateTips( t )
	{
		tips.text( t ).addClass( "ui-state-highlight" );
		setTimeout(function() {
			tips.removeClass( "ui-state-highlight", 1500 );
		}, 500 );
	}

	function checkSet( o, n ) {
		if ( o.val().length > 0) {
			return true;
		}
		else {
			o.addClass( "ui-state-error" );
			updateTips( n + " must be specified" );
			return false;
		}
	}

	var buttons =
		{
			"Ok": function() {
				if ( validateForm() )
				{
					var data = { action: 'update_project', projectId: id, name: name.val(), command: command.val(), trigger: trigger.val() };
					jQuery.post("carson_api.php", data, projectActionCallback );
					$( this ).dialog( "close" );
				}
			},
			Cancel: function() {
				$( this ).dialog( "close" );
			}
		};

	var dialog = $( "#dialog-form" );

	dialog.dialog( "option", "title", "Edit Project" );
	dialog.dialog( "option", "buttons", buttons );
	dialog.dialog( "open" );

	return false;
}

function projectDeleteButtonClick( event)
{

	var id = $(event.target).parents('li').find('input').val();

	$( "#dialog-confirm" ).dialog({
			resizable: false,
			height:200,
			modal: true,
			buttons: {
				"Delete project": function() {
					$( this ).dialog( "close" );
					jQuery.post("carson_api.php", { action: 'delete_project', projectId: id }, projectActionCallback );
				},
				Cancel: function() {
					$( this ).dialog( "close" );
				}
			}
		});

	return false;

}

function createProjectButtonClick()
{

	var name    = $( "#name" );
	var command = $( "#command" );
	var trigger = $( "#trigger" );

	var buttons =
		{
			"Create Project": function() {
				if ( validateForm() ) {
					var data = { action: 'create_project', name: name.val(), command: command.val(), trigger: trigger.val() };
					jQuery.post("carson_api.php", data, projectActionCallback );
					$( this ).dialog( "close" );
				}
			},
			Cancel: function() {
				$( this ).dialog( "close" );
			}
		};

	var dialog = $( "#dialog-form" );
	dialog.dialog( "option", "title", "Create Project" );
	dialog.dialog( "option", "buttons", buttons );
	dialog.dialog( "open" );

}

$(function() {

	var name    = $( "#name" );
	var command = $( "#command" );
	var trigger = $( "#trigger" );

	var allFields = $( [] ).add( name ).add( command ).add( trigger );

	$( "#dialog-form" ).dialog({
			autoOpen: false,
			width:  500,
			height: 600,
			modal: true,
			close: function() {
				allFields.val( "" ).removeClass( "ui-state-error" );
			}
		});

	// Create the button to open the new project dialog.
	$("#create_project").button().click( createProjectButtonClick );

	updateProjects();

	window.setInterval(update, 1000);
	window.setInterval(updateProjects, 5000);

    $('#logContainer .header').live('click', function(){
        $(this).siblings('pre').toggle();

        /*console.log($(this).siblings('.output'))
        var outputContainer = $(this).siblings('.output:eq(0)');

        if(outputContainer.hasClass('outputVisible')) {
            outputContainer.removeClass('outputVisible').hide();
            console.log(1)
        }
        else {
            outputContainer.addClass('outputVisible').show();
            console.log(2)
        }*/
    })

});
