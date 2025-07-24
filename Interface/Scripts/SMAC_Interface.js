//=========================================================
//
//     FILE : SMAC_Interface.js
//
//  PROJECT : SMAC Interface
//
//  PURPOSE : Main SMAC Relayer/Node/Device Communications
//
//   AUTHOR : Bill Daniels
//            Copyright 2014-2025, D+S Tech Labs, Inc.
//            All Rights Reserved
//
//=========================================================

//--- Globals ---------------------------------------------

const AppVersion = '── 2025.07.21b ──';
const Debugging  = true;  // Set to false for production use

let TotalPages  = 0;
let PagesLoaded = false;

const SerialPortSettings =
{
  baudRate    : 115200,
  dataBits    : 8,
  stopBits    : 1,
  parity      : 'none',
  bufferSize  : 1024,
  flowControl : 'none'
};

const EOL = '\r\n';

// SerialPort object (to be created if supported)
let SMACPort = undefined;

let DTInterval = undefined;
let DataString = '';

//--- Node Array: ---
const MaxNodes = 20;  // Limited to 20 due to ESP-NOW peer limit
let   Nodes = [MaxNodes];  // Holds Node objects:
                           // {                                        ┌─
                           //   name,                                  │  {
                           //   version,                               │    name,
                           //   macAddress,                            │    version,
                           //   numDevices,                            │    ipEnabled,
                           //   devices[],  - array of Device objects ─┤    ppEnabled,
                           //   monitor,                               │    rate
                           //   lastMessageTime                        │  }
                           // }                                        └─
                           // The index is the nodeID

//--- Startup ---------------------------------------------

try
{
  // Set version strings
  $('#title_Version'  ).html (AppVersion);
  $('#dsSplashVersion').html (AppVersion);

  // Check if this browser supports serial communication
  if (!('serial' in navigator) || navigator.serial == undefined)
    $('#smacPageArea').html ('<h1 style="color:#C00000; text-shadow:1px 1px 1px #000000; text-align:center">' +
                             'This browser does not support serial communications.<br>Please use the Chrome or Edge browser.</h1>');
  else
  {
    // Clear the Node array
    for (let i=0; i<MaxNodes; i++)
      Nodes[i] = undefined;

    // Create a SerialPort object
    SMACPort = new SerialPort (async (dataString) => { await ProcessRelayerMessage (dataString); }, RelayerDisconnected);

    // Show the Relayer connection screen
    $('#dsSplash').show ();

    // Close serial port before exit
    window.addEventListener ('beforeunload', async (event) =>
    {
      StopEvent (event);
      event.returnValue = '';

      await Unload ();

      return undefined;
    });

    // Disable right-click context menu for entire SMAC console
    document.addEventListener ('contextmenu', (event) => { event.preventDefault(); });
  }
}
catch (ex)
{
  ShowException (ex);
}

//--- Unload ----------------------------------------------

async function Unload ()
{
  try
  {
    if (DTInterval != undefined)
      clearInterval (DTInterval);

    await CloseDataLog ();

    if (SMACPort != undefined)
      await SMACPort.Close ();
  }
  catch (ex)
  {
    ShowException (ex);
  }
}

//--- UpdateConnectionBox ---------------------------------

function UpdateConnectionBox ()
{
  try
  {
    if (SMACPort.IsOpen ())
    {
      const cb = $('#connectionBox');
      cb.css ('color', '#F0F0F0');
      cb.css ('background-color', '#008000');
      $('#cbStatus').text ('Connected');
    }
    else
    {
      const cb = $('#connectionBox');
      cb.css ('color', '#808080');
      cb.css ('background-color', '#303030');
      $('#cbStatus').text ('Not Connected');
    }
  }
  catch (ex)
  {
    ShowException (ex);
  }
}

//--- ConnectToRelayer ------------------------------------

async function ConnectToRelayer ()
{
  try
  {
    await ConnectToRelayer_Async ();

    if (SMACPort.IsOpen ())
    {
      // Hide splash
      $('#dsSplash').hide ();





      // GoFullscreen ();





      window.devicePixelRatio = 1.0;  // set pixel density to 1

      // Load All Tabbed Pages
      LoadContentPages (() =>
      {
        // Trigger custom event if browser is resized.
        // This is so custom SMAC Widgets can listen for it and resize themselves.
        window.addEventListener ('resize', (event) => { $(document.body).trigger ('browserResized'); });

        // Show nav buttons, diagnostics button and status bar
        $('#navButtons'   ).css ('display', 'inline-block');
        $('#diagAndCBox'  ).css ('display', 'inline-block');
        $('#smacStatusBar').css ('display', 'flex'        );

        UpdateConnectionBox ();

        // Update current date/time once per minute
        SetDateTime ();
        DTInterval = setInterval (SetDateTime, 60000);

        // Click on the first nav button
        $('.navButton').first().trigger ('pointerdown');

        // Wait for Relayer
        setTimeout (async () =>
        {
          // Request full system info from the Relayer
          await Send_UItoRelayer (0, 0, 'SYSI');  // System Info command

          // Run 'UserStartup()' (defined by User at bottom of index.html)
          UserStartup ();
        }, 1000);
      });
    }
  }
  catch (ex)
  {
    ShowException (ex);
  }
}

//--- ConnectToRelayer_Async ------------------------------

async function ConnectToRelayer_Async ()
{
  try
  {
    // Select serial port
    await SMACPort.ChoosePort ()
    .then (async () =>
    {
      // Open serial port
      await SMACPort.Open (SerialPortSettings)
      .then (async portOpened =>
      {
        // Check if port is opened
        if (portOpened)
        {
          console.info ('Port is open. Starting Read Loop ...');

          // Start read loop
          // We disconnected if this read loop finishes
          SMACPort.ReadLoop ();
        }
      })
      .catch (ex =>
      {
        // Open() failed
        PopupMessage ('SMAC Interface', 'Unable to connect to Relayer.');
      });
    })
    .catch (ex =>
    {
      // Ignore "NotFoundError" (No port choosen)
    });
  }
  catch (ex)
  {
    ShowException (ex);
  }
}

//--- RelayerDisconnected ---------------------------------

function RelayerDisconnected ()
{
  try
  {
    UpdateConnectionBox ();
    $('#smacPageArea').html ('<div class="relayerDisconnected">Relayer Disconnected</div>');

    setTimeout (() => { location.reload (); }, 1000);
  }
  catch (ex)
  {
    ShowException (ex);
  }
}

//--- SetDateTime -----------------------------------------

function SetDateTime ()
{
  try
  {
    const now = new Date ();

    let value = now.toDateString();
    $("#statusBar_Date").html (value);

    value = now.toTimeString().substring(0, 5);
    $("#statusBar_Time").html (value);






    // Also check if all Nodes are still alive
    // ...





  }
  catch (ex)
  {
    ShowException (ex);
  }
}

//--- ProcessRelayerMessage -------------------------------

async function ProcessRelayerMessage (dataString)
{
  try
  {
    // Normal incoming Data Strings from the Relayer have the
    // following format: (fields are separated with the '|' char)
    //
    //   ┌─────────────────── 2-char nodeID (00-19)
    //   │  ┌──────────────── 2-char deviceID (00-99)
    //   │  │     ┌────────── Variable length timestamp - usually millis()
    //   │  │     │       ┌── Variable length value string
    //   │  │     │       │   (this can be a numerical value or a text/error message)
    //   │  │     │       │
    //   nn|dd|timestamp|value
    //
    // There are two special messages from the Relayer:
    //
    //   NODE|...     Indicates that a new Node has attached to the system
    //   ERROR:...    Indicates a Relayer error

    if (Debugging)
      console.info ('--> ' + dataString);

    // Check if new Node connected: NODE|nodeID
    if (dataString.startsWith ('NODE|'))
    {
      const nodeID    = dataString.substring (5, 7);
      const nodeIndex = parseInt (nodeID);

      if (nodeIndex < MaxNodes)
      {
        PopupBar ('New Node connected to Relayer', 2000);

        if (Debugging)
          console.info ('((( Node ' + nodeID + ' connected )))');

        // Request Node Info: nn|00|timestamp|NOINFO|name|version|macAddress|numDevices
        await Send_UItoRelayer (nodeIndex, 0, 'GNOI');

        // Request Device Info: nn|dd|timestamp|DEINFO|name|version|ipEn(Y/N)|ppEn(Y/N)|rate (for each Device)
        await Send_UItoRelayer (nodeIndex, 0, 'GDEI');
      }

      return;
    }

    // Error messages from the Relayer have no Node or Device associated with them.
    // Their message starts with 'ERROR:' ...
    // Show any Relayer Errors
    if (dataString.startsWith ('ERROR:'))
    {
      PopupMessage ('SMAC Relayer/Node Error', dataString.substring (6));
      return;
    }

    // All other messages should be a Node/Device Data String: nodeID|deviceID|timestamp|value
    //   nodeID    = 2-digits 00-19
    //   deviceID  = 2-digits 00-99
    //   timestamp = n digits
    //   value     = Device data or message

    const fields = dataString.split ('|');
    if (fields.length < 4)
      return;

    // Parse Data String
    const nodeIndex   = parseInt (fields[0]);
    const deviceIndex = parseInt (fields[1]);
    const timestamp   = BigInt   (fields[2]);
    const value       = dataString.substring (fields[2].length + 7);  // Because value may also have '|' characters and fields.
                                                                      // We need the whole value string including '|'s and fields.
    if (Diagnostics.DataLogging)
      Diagnostics.LogToMonitor (nodeIndex, '──▶ ' + dataString);







    // // Mark last time since we heard from Node
    // Node[nodeIndex].lastMessageTime = timestamp;







    // First, update Widgets with actual device data values
    // They start with a dash or a digit
    if (value[0] == '-' || (value[0] >= '0' && value[0] <= '9'))
    {
      // Update all UI Widgets with device data
      // These events are handled by SMAC Widgets
      $(document.body).trigger ('deviceData', [ nodeIndex, deviceIndex, timestamp, value ]);
      return;
    }

    //=====================================================================
    // Handle non-numeric (message) values:
    //   NOINFO=
    //   DEINFO=
    //   NONAME=
    //   DENAME=
    //   RATE=
    //   IP Enabled
    //   IP Disabled
    //   IP Performed
    //   PP Enabled
    //   PP Disabled
    //   VER=
    //   FILES=
    //   FILE=
    //   ERROR=
    //   PONG
    //=====================================================================

    if (value.startsWith ('NOINFO='))
    {
      // Add new Node if it does NOT exist already
      if (Nodes[nodeIndex] == undefined)
      {
        const newNode = {
                          name       : 'not set',
                          version    : 'not set',
                          macAddress : 'not set',
                          numDevices : 0,
                          devices    : [],          // Device objects { name, version, ipEnabled, ppEnabled, rate }
                          monitor    : undefined
                        };

        Nodes[nodeIndex] = newNode;
      }

      // Update Node info fields: name, version, macAddress, numDevices
      Nodes[nodeIndex].name       = fields[3].substring(7);
      Nodes[nodeIndex].version    = fields[4];
      Nodes[nodeIndex].macAddress = fields[5];
      Nodes[nodeIndex].numDevices = parseInt (fields[6]);

      // Update the Diagnostics UI
      Diagnostics.BuildSystem ();
      Diagnostics.LogToMonitor (nodeIndex, 'Node ' + nodeIndex.toString() + ' connected.');



      // // Trigger an event to inform anyone that a new Node was added
      // $(document.body).trigger ('newNode', [ nodeIndex ]);



    }

    else if (value.startsWith ('DEINFO='))
    {
      // Make sure Node exists in Diagnostics
      if (Nodes[nodeIndex] != undefined)
      {
        // One Device per 'DEINFO=...' message
        // Fill five Device fields: name, version, ipEnabled, ppEnabled, rate

        const deviceArray  = Nodes[nodeIndex].devices;
        const deviceFields = value.substring(7).split ('|');
        deviceArray[deviceIndex] = { name:deviceFields[0], version:deviceFields[1], ipEnabled:deviceFields[2], ppEnabled:deviceFields[3], rate:deviceFields[4] };

        Diagnostics.UpdateDevices (nodeIndex);  // Update the UI Device fields of the Node Block in Diagnostics

        // Update status bar Device count
        let totalDevices = 0;
        Nodes.forEach ((node) =>
        {
          if (node != undefined)
            totalDevices += node.numDevices;
        });

        $("#statusBar_Devices").html (totalDevices.toString());
      }
    }

    else if (value.startsWith ('NONAME='))
    {
      Nodes[nodeIndex].name = value.substring(7);
      Diagnostics.UpdateNodeBlock (nodeIndex);
    }

    else if (value.startsWith ('DENAME='))
    {
      Nodes[nodeIndex].devices[deviceIndex].name = value.substring(7);
      Diagnostics.UpdateDevices (nodeIndex);
    }

    else if (value.startsWith ('RATE='))
    {
      Nodes[nodeIndex].devices[deviceIndex].rate = value.substring(5);
      Diagnostics.UpdateDevices (nodeIndex);
    }

    else if (value == 'IP Enabled')
    {
      Nodes[nodeIndex].devices[deviceIndex].ipEnabled = 'Y';
      Diagnostics.UpdateDevices (nodeIndex);
    }

    else if (value == 'IP Disabled')
    {
      Nodes[nodeIndex].devices[deviceIndex].ipEnabled = 'N';
      Diagnostics.UpdateDevices (nodeIndex);
    }

    else if (value == 'PP Enabled')
    {
      Nodes[nodeIndex].devices[deviceIndex].ppEnabled = 'Y';
      Diagnostics.UpdateDevices (nodeIndex);
    }

    else if (value == 'PP Disabled')
    {
      Nodes[nodeIndex].devices[deviceIndex].ppEnabled = 'N';
      Diagnostics.UpdateDevices (nodeIndex);
    }

    else if (value == 'NVER=')
    {
      Nodes[nodeIndex].version = value.substring(5);
      Diagnostics.BuildSystem ();
    }

    else if (value == 'DVER=')
    {
      Nodes[nodeIndex].devices[deviceIndex].version = value.substring(5);
      Diagnostics.UpdateDevices (nodeIndex);
    }

    else if (value.startsWith ('FILES='))
    {
      // TODO: List of files
    }

    else if (value.startsWith ('FILE='))
    {
      // TODO: File contents
    }

    else if (value.startsWith ('ERROR='))
    {
      // Error messages from a Node or Device post their error in the value field.
      // The message has the format:
      //   nodeID|deviceID|timestamp|ERROR=...

      // Log Node/Device Error message
      Diagnostics.LogToMonitor (nodeIndex, value.substring(6));
    }

    else if (value.startsWith ('PONG'))
    {
      // Log PONG received
      Diagnostics.LogToMonitor (nodeIndex, 'PONG Received');
    }
  }
  catch (ex)
  {
    ShowException (ex);
  }
}

//--- Send_UItoRelayer ------------------------------------

async function Send_UItoRelayer (nodeIndex, deviceIndex, commandString, paramString)
{
  try
  {
    // Command Format: nodeID|deviceID|command|params
    // where:
    //   nodeID   = 2-digits 00-19
    //   deviceID = 2-digits 00-99
    //   command  = 4-chars (usually caps)
    //   params   = optional parameters (null terminated string)

    const nodeID   = nodeIndex  .toString().padLeft ('0', 2);
    const deviceID = deviceIndex.toString().padLeft ('0', 2);
    const command  = commandString.substring (0, 4).padRight (' ', 4);
    const params   = (paramString == undefined || paramString == '') ? '' : '|' + paramString;

    const fullUIMessage = nodeID + '|' + deviceID + '|' + command + params;

    try
    {
      await SMACPort.Send (fullUIMessage + '\n');
    }
    catch (ex)
    {
      console.error (ex);
      return;
    }

    if (Debugging)
      console.info ('<-- ' + fullUIMessage);

    // Log message to the appropriate Node monitor
    if (Diagnostics.DataLogging)
      Diagnostics.LogToMonitor (nodeIndex, '◀── ' + fullUIMessage);
  }
  catch (ex)
  {
    ShowException (ex);
  }
}



// //--- BroadcastUIMessage ----------------------------------
//
// async function BroadcastUIMessage (commandString)
// {
//   try
//   {
//     await Send_UItoRelayer (0, 0, 'CAST', commandString);
//   }
//   catch (ex)
//   {
//     ShowException (ex);
//   }
// }
//
// //--- GetFileList -----------------------------------------
//
// async function GetFileList (path, extension)
// {
//   try
//   {
//     await Send_UItoRelayer (0, 0, 'GFLI', path + '|' + extension);
//   }
//   catch (ex)
//   {
//     ShowException (ex);
//   }
// }
//
// //--- GetFileContents -------------------------------------
//
// async function GetFileContents (path)
// {
//   try
//   {
//     await Send_UItoRelayer (0, 0, 'GFCO', path);
//   }
//   catch (ex)
//   {
//     ShowException (ex);
//   }
// }
//
// //--- PutFileContents -------------------------------------
//
// async function PutFileContents (path, contents)
// {
//   try
//   {
//     await Send_UItoRelayer (0, 0, 'PFIC', path + '|' + contents);
//   }
//   catch (ex)
//   {
//     ShowException (ex);
//   }
// }



//--- CloseDataLog-----------------------------------------

async function CloseDataLog ()
{
  try
  {
    // Close any Data Log files that are recording




  }
  catch (ex)
  {
    ShowException (ex);
  }
}
