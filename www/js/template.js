/** Super simple browser side template. */
function templateRender(templateId, data)
{
	var html = $(templateId).html();
	for (name in data)
	{
		var regex = new RegExp('\\${' + name + '}', "gm");
		html = html.replace(regex, data[name]);
	}
	return html;
}