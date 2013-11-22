status = { }

local beginStr = [[<div class="element">
    <div class="header">%s</div>
    <pre class="output">]]

local endStr = [[
    </pre>
    <div class="status">%s</div>
</div>]]

local results = { total = 0, failures = 0 }

function status.runstep(step, work)

    print(string.format(beginStr, step))
    local result = work()
    results.total = results.total + 1
    results.failures = results.failures + ((result == true) and 0 or 1)
    print(string.format(endStr, (result == true) and "success" or "failed"))

end

function status.getresults()
    return results
end