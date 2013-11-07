status = { }

local beginStr = [[<div class="element">
    <div class="header">%s</div>
    <pre class="output">]]

local endStr = [[
    </pre>
    <div class="status">%s</div>
</div>]]

function status.runstep(step, work)

    print(string.format(beginStr, step))
    local result = work()
    print(string.format(endStr, (result == true) and "success" or "failed"))

end