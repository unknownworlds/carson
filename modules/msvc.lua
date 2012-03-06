msvc = { } 

local devenv = os.getenv("MSVC_EXE") or error("MSVC_EXE must be defined")

function msvc.build(solution, config)
	
	local command = [[""]] .. devenv .. [[" "]] .. solution .. [[" /build "]] .. config .. [[""]]
	local result = os.capture(command, true)
	
	-- Check the results of the build
	local succeeded, failed, uptodate, skipped =
		string.match(result, "========== Build: (%d+) succeeded, (%d+) failed, (%d+) up%-to%-date, (%d+) skipped ==========")
	
	return {
		succeeded = tonumber(succeeded),
		failed    = tonumber(failed),
		uptodate  = tonumber(uptodate),
		skipped   = tonumber(skipped),
		result    = result
	}

end

return msvc