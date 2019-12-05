
#include <iostream>
#include <iomanip>
#include <string>
#include <argh/argh.h>

#include "RTProtocol.h"

namespace qtmc 
{

    struct RtCommandCompleteTrigger 
    {
        std::string resultString = "";
        CRTPacket::EEvent eventType = CRTPacket::EventNone;
    };

    struct RtCommandResult
    {
        bool receivedMatchingTrigger = false;
        bool receivedErrorString = false;
        bool receivedPacketError = false;
        bool sendCommandFailed = false;
        std::string errorString = "";
        
        bool has_error() const
        {
            return  receivedErrorString == true || sendCommandFailed == true || receivedPacketError == true;
        }

        bool has_satisfied_result() const
        {
            return has_error() == false && receivedMatchingTrigger == true;
        }
    };
    
    RtCommandResult send_command_await_event(CRTProtocol &rtProtocol, const std::string& commandString, const std::vector<RtCommandCompleteTrigger>& triggers)
    {
        RtCommandResult result;
        if (!rtProtocol.SendCommand(commandString.c_str()))
        {
            result.sendCommandFailed = true;
            result.errorString = "Send command failed.";
        }

        std::vector<CRTPacket::EEvent> rxEvents;
        std::vector<std::string> rxResultStrings;
        std::vector<std::string> rxPacketErrors;

        while (!result.has_error() && !result.has_satisfied_result())
        {
            //Get packet
            {

                CRTPacket::EPacketType eType;
                if (rtProtocol.ReceiveRTPacket(eType, false) <= 0)
                {
                    continue;
                }

                auto packet = rtProtocol.GetRTPacket();
                switch (eType)
                {
                case CRTPacket::EPacketType::PacketError:
                    rxPacketErrors.emplace_back(packet->GetErrorString());
                    break;
                case CRTPacket::EPacketType::PacketCommand:
                    rxResultStrings.emplace_back(packet->GetCommandString());
                    break;
                case CRTPacket::EPacketType::PacketEvent:
                    CRTPacket::EEvent evt;
                    packet->GetEvent(evt);
                    rxEvents.emplace_back(evt);
                    break;
                case CRTPacket::EPacketType::PacketXML:
                case CRTPacket::EPacketType::PacketData:
                case CRTPacket::EPacketType::PacketNoMoreData:
                case CRTPacket::EPacketType::PacketC3DFile:
                case CRTPacket::EPacketType::PacketDiscover:
                case CRTPacket::EPacketType::PacketQTMFile:
                case CRTPacket::EPacketType::PacketNone:
                    break;
                }
            }

            //Match triggers
            {

                if (rxPacketErrors.size() > 0) //catch straight up errors
                {
                    result.receivedPacketError = true; 
                    result.errorString = rxPacketErrors.back();
                }
                else
                {
                    const std::string resultString = rxResultStrings.size() > 0 ? rxResultStrings.back() : ""; 
                    
                    //find complete match
                    auto matchingTrigger = std::find_if(triggers.begin(), triggers.end(), [&](const auto& trigger) {
                        return resultString == trigger.resultString 
                            && ( trigger.eventType == CRTPacket::EventNone || std::find(rxEvents.begin(), rxEvents.end(), trigger.eventType) != rxEvents.end() );
                    });


                    if (matchingTrigger != triggers.cend())
                    {
                        result.receivedMatchingTrigger = true;
                    }
                    else
                    {
                        //no matches, look for invalid result strings
                        if (resultString != "")
                        {
                            auto incorrectResultString = std::find_if(triggers.begin(), triggers.end(), [&](const auto& trigger) {
                                return resultString == trigger.resultString;
                            }) == triggers.end();

                            if (incorrectResultString == true)
                            {
                                result.receivedErrorString = true;
                                result.errorString = resultString;
                            }
                        }
                     }
                }
            }
        }
        return result;
    }

    enum class OperationResult {
        Ok,
        FailedToConnect,
        FailedToTakeControl,
        FailedToStartCapture,
        FailedToCreateNewMeasurement,
        FailedToCloseMeasurement,
        FailedToSaveMeasurement,
        FailedToLoadMeasurement,
        FailedToStopCapture,
    };

    std::string operation_result_to_string(const OperationResult& result)
    {
        //Todo: Exhaustive switch compiler error
        switch (result)
        {
        case OperationResult::Ok: return "Ok.";
        case OperationResult::FailedToConnect: return "Failed to connect.";
        case OperationResult::FailedToTakeControl: return "Failed to take control.";
        case OperationResult::FailedToStartCapture: return "Failed to start capture.";
        case OperationResult::FailedToCreateNewMeasurement: return "Failed to create new measurement.";
        case OperationResult::FailedToSaveMeasurement: return "Failed to save measurement.";
        case OperationResult::FailedToLoadMeasurement: return "Failed to load measurement.";
        case OperationResult::FailedToStopCapture: return "Failed to stop capture.";
        case OperationResult::FailedToCloseMeasurement: return "Failed to close measurement.";
        default: return "Failed.";
        }
    }

    struct CommandResult
    {
        OperationResult result  = OperationResult::Ok;
        std::string errorMessage = "";
    };

    std::ostream& operator<<(std::ostream& os, const CommandResult& r)
    {
        if (r.result == OperationResult::Ok) 
        {}
        else 
        {
            os << qtmc::operation_result_to_string(r.result) << " (" << r.errorMessage << ")" << std::endl;
        }
        return os;
    }

    struct ConnectionOptions 
    {
        std::string password = "gait1";
        std::string ip = "localhost";
        unsigned short basePort = 22222;
        const int majorVersion = 1;
        const int minorVersion = 18;
        const bool bigEndian = false;
    };

    bool command_take_control(CRTProtocol& rtProtocol, const ConnectionOptions& connectionOptions, CommandResult &result) {
        if (!rtProtocol.Connect(
            connectionOptions.ip.c_str(),
            connectionOptions.basePort,
            0,
            connectionOptions.majorVersion,
            connectionOptions.minorVersion,
            connectionOptions.bigEndian))
        {
            result = {
                OperationResult::FailedToConnect,
                std::string(rtProtocol.GetErrorString())
            };
            return false;
        }

        if (!rtProtocol.IsControlling() && !rtProtocol.TakeControl(connectionOptions.password.c_str()))
        {
            result = {
                OperationResult::FailedToTakeControl,
                std::string(rtProtocol.GetErrorString())
            };
            return false;
        }
        return true;
    }
    
    void command_release_control(CRTProtocol& rtProtocol)
    {
        rtProtocol.ReleaseControl();
        rtProtocol.Disconnect();
    }
    
    bool command_close(CRTProtocol& rtProtocol, CommandResult& result)
    {
        auto r = qtmc::send_command_await_event(rtProtocol, "Close", {
            { "Closing connection", CRTPacket::EEvent::EventConnectionClosed },
            { "No connection to close", CRTPacket::EEvent::EventNone },
            { "File closed", CRTPacket::EEvent::EventNone },
        });

        if (r.has_error())
        {
            result = {
                OperationResult::FailedToCloseMeasurement,
                std::string(r.errorString)
            };
            return false;
        }
        else 
        {
            return true;
        }
    }

    bool command_new(CRTProtocol& rtProtocol, CommandResult& result) 
    {
        auto r = qtmc::send_command_await_event(rtProtocol, "New", {
            {  "Creating new connection"  , CRTPacket::EEvent::EventConnected },
            {  "Already connected"  , CRTPacket::EEvent::EventNone },
        });

        if (r.has_error())
        {
            result = {
                OperationResult::FailedToCreateNewMeasurement,
                std::string(r.errorString)
            };
            return false;
        }
        return true;
    }

    bool command_start(CRTProtocol& rtProtocol, CommandResult& result)
    {
        auto r = qtmc::send_command_await_event(rtProtocol, "Start", {
            {  "Starting measurement"  , CRTPacket::EEvent::EventCaptureStarted },
            {  "Measurement is already running"  , CRTPacket::EEvent::EventNone },
         });

        if (r.has_error())
        {
            result = {
                OperationResult::FailedToStartCapture,
                std::string(r.errorString)
            };
            return false;
        }
        return true;
    }

    bool command_save(CRTProtocol& rtProtocol, const std::string& filename,  CommandResult& result)
    {
        if (!rtProtocol.SaveCapture(filename.c_str(), true)) 
        {
            result = {
                OperationResult::FailedToSaveMeasurement,
                std::string(rtProtocol.GetErrorString())
            };
            return false;
        }
        return true;
    }

    bool command_load(CRTProtocol& rtProtocol, const std::string& filename, CommandResult& result)
    {
        if (!rtProtocol.LoadCapture(filename.c_str())) {
            result =  {
                OperationResult::FailedToLoadMeasurement,
                std::string(rtProtocol.GetErrorString())
            };
            return false;
        }
        return true;
    }

    bool command_stop(CRTProtocol& rtProtocol, CommandResult& result)
    {
        auto r = qtmc::send_command_await_event(rtProtocol, "Stop", {
            { "No measurement is running", CRTPacket::EEvent::EventNone },
            { "Stopping measurement" , CRTPacket::EEvent::EventCaptureStopped }
        });

        if (r.has_error()) {
            result = {
                OperationResult::FailedToStopCapture,
                r.errorString
            };
            return false;
        }

        return true;
    }

    CommandResult start_capture(const ConnectionOptions& connectionOptions)
    {
        CommandResult result;
        CRTProtocol rtProtocol;
        if (!command_take_control(rtProtocol, connectionOptions, result)) { return result; }
        if (!command_close(rtProtocol, result)) { return result; }
        if (!command_new(rtProtocol, result)) { return result; }
        if (!command_start(rtProtocol, result)) { return result; }
        command_release_control(rtProtocol);
        return result;
    };

    CommandResult save_measurement(const ConnectionOptions& connectionOptions, const std::string& filename)
    {
        CommandResult result, _;
        CRTProtocol rtProtocol;
        if (!command_take_control(rtProtocol, connectionOptions, result)) { return result; }
        if (!command_stop(rtProtocol, _)) { /*this can fail, we just want to be sure the measurement is stopped*/ }
        if (!command_save(rtProtocol, filename, result)) { return result; }
        command_release_control(rtProtocol);
        return result;
    };

    CommandResult stop_capture(const ConnectionOptions& connectionOptions, const std::string& filename)
    {
        CommandResult result;
        CRTProtocol rtProtocol;
        if (!command_take_control(rtProtocol, connectionOptions, result)) { return result; }
        if (!command_stop(rtProtocol, result)) { return result; }
        if (filename != "" && !command_save(rtProtocol, filename, result)) { return result; }
        command_release_control(rtProtocol);
        return result;
    };
}

bool app_logic(const qtmc::ConnectionOptions& connectionOptions, const std::string& operation, const std::string& filename, qtmc::CommandResult& result)
{
    result = qtmc::CommandResult();
    if (operation == "start")
    {
        result = qtmc::start_capture(connectionOptions);
    }
    else if (operation == "stop")
    {
        result = qtmc::stop_capture(connectionOptions, filename);
    }
    else if (operation == "save" && filename != "")
    {
        result = qtmc::save_measurement(connectionOptions, filename);
    }

    else
    {
        std::cout << std::endl;
        const char* version = "v0.0.2";
        const int C0 = 4, C1 = 11, C2 = 18;

        std::cout << std::setw(C0) << "" << "QTM Control (qtmc) " << version << std::endl;
        std::cout << std::endl;

        std::cout << "Description:" << std::endl;
        std::cout << std::setw(C0) << "" << "This program provides command line automation control of QTM" << std::endl;
        std::cout << std::endl;

        std::cout << "Usage:" << std::endl;
        std::cout << std::setw(C0) << "" << "qtmc (start|stop [<filename>]|save <filename>) [--host=<hostname or ip>] [--port=<number>] [--password=<text>]" << std::endl;
        std::cout << std::endl;

        std::cout << "Arguments:" << std::endl;
        std::cout << std::right << std::setw(C1) << "start" << std::left << std::setw(C2) << "" << "Creates a new measurement and starts capture." << std::endl;
        std::cout << std::right << std::setw(C1) << "stop" << std::left << std::setw(C2) << " [<filename>] " << "Stops a running capture. If <filename> is provided, the capture is saved as <filename>" << std::endl;
        std::cout << std::right << std::setw(C1) << "save" << std::left << std::setw(C2) << " <filename> " << "Saves a stopped capture as <filename>" << std::endl;
        std::cout << std::right << std::setw(C1) << "--host" << std::left << std::setw(C2) << "=<hostname or ip> " << "QTM RT endpoint address, default=localhost." << std::endl;
        std::cout << std::right << std::setw(C1) << "--port" << std::left << std::setw(C2) << "=<number> " << "QTM RT endpoint port, default=22222." << std::endl;
        std::cout << std::right << std::setw(C1) << "--password" << std::left << std::setw(C2) << "=<text>" << "Specifies the QTM \"take control\" password. default=****." << std::endl;
        std::cout << std::right << std::setw(C1) << "--help" << std::left << std::setw(C2) << "" << "Shows this message." << std::endl;
        std::cout << std::endl;

        std::cout << "Notice:" << std::endl;
        std::cout << std::setw(C0) << "" << "QTM must be started, and a project must be loaded for the RT protocol to be active." << std::endl;
        std::cout << std::endl;
    }
    return result.result == qtmc::OperationResult::Ok;
}

int main(const int argc, const char* argv[])
{
    argh::parser cmdl(argc, argv);
    auto args = cmdl.pos_args();
    
    std::string operation = args.size() > 1 ? args[1] : "";
    std::string filename = args.size() > 2 ? args[2] : "";

    auto connectionOptions = qtmc::ConnectionOptions();

    cmdl("--host") >> connectionOptions.ip;
    cmdl("--port") >> connectionOptions.basePort;
    cmdl("--password") >> connectionOptions.password;

    qtmc::CommandResult result;
    if (!app_logic(connectionOptions, operation, filename, result)) {
        std::cerr << result;
        return 1;
    }
    else 
    {
        return 0;
    }

}
