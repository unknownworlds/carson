mail = { }

local mailexe      = os.getenv("MAIL_EXE")    or error("MAIL_EXE must be defined")
local mailserver   = os.getenv("MAIL_SERVER") or error("MAIL_SERVER must be defined")
local mailport     = os.getenv("MAIL_PORT")   or error("MAIL_PORT must be defined")
local mailuser     = os.getenv("MAIL_USER")
local mailpassword = os.getenv("MAIL_PASSWORD")

-- Sends an e-mail to the specified recipient.
function mail.send(to, subject, message)

    local command = [["]] .. mailexe .. [["]]
    
    -- Create a file containing the message we want to send.
    local messageFileName = os.tmpname()
    local messageFile = io.open(messageFileName, "w")
    messageFile:write(message)
    messageFile:close()
    
    command = command .. [[ "]] .. messageFileName .. [["]]
    
    command = command .. [[ -to ]] .. to
    command = command .. [[ -subject "]] .. subject .. [["]]
    command = command .. [[ -server ]] .. mailserver
    command = command .. [[ -port ]] .. mailport
    
    if mailuser then
        command = command .. [[ -f ]] .. mailuser
        command = command .. [[ -u ]] .. mailuser
    end
        
    if mailpassword then
        command = command .. [[ -pw ]] .. mailpassword
    end
    
    command = command .. [[ -html ]]

    os.capture([["]] .. command .. [["]])
    os.remove(messageFileName)
    
end

local function mailerror(mailToAddress)

    if success then
        return
    end

    mail.send(mailToAddress, "Build Failure", _LOG)

end

function mail.onerror(mailToAddress)
    os.atexit(function(success) mailerror(success, mailToAddress) end)
end

return mail