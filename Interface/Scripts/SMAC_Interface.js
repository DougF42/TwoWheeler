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

const AppVersion = '── 2025.11.12 ──';
const Debugging  = false;  // Set to false for production use

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

let SMACPort         = undefined;  // SerialPort object (to be created if supported)
let DateTimeInterval = undefined;  // Used to update the Status Bar Date/Time

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
                           //   lastMsgTime                            │  }
                           // }                                        └─
                           // The index is the nodeID

//--- Status Bar (object) ---------------------------------

const StatusBar =
{
  //--- SetState ---
  SetState : function (value)
  {
    try
    {
      $("#statusBar_State").html (value);
    }
    catch (ex)
    {
      ShowException (ex);
    }
  },

  //--- SetMessage ---
  SetMessage : function (message, color)
  {
    try
    {
      $("#statusBar_Message").html (message);
      if (color == undefined)
        $("#statusBar_Message").css ('color', "var(--light)");
      else
        $("#statusBar_Message").css ('color', color);
    }
    catch (ex)
    {
      ShowException (ex);
    }
  },

  //--- SetNumNodes ---
  SetNumNodes : function (numNodes)
  {
    try
    {
      $("#statusBar_Nodes").html (numNodes.toString());
    }
    catch (ex)
    {
      ShowException (ex);
    }
  },

  //--- SetNumDevices ---
  SetNumDevices : function (numDevices)
  {
    try
    {
      $("#statusBar_Devices").html (numDevices.toString());
    }
    catch (ex)
    {
      ShowException (ex);
    }
  },

  //--- SetDateTime ---
  SetDateTime : function ()
  {
    try
    {
      const now = new Date ();

      let value = now.toDateString();
      $("#statusBar_Date").html (value);

      value = now.toTimeString().substring(0, 5);
      $("#statusBar_Time").html (value);
    }
    catch (ex)
    {
      ShowException (ex);
    }
  }
};

//--- Startup ---------------------------------------------

try
{
  // Set version strings
  $('#title_Version'  ).html (AppVersion);
  $('#dsSplashVersion').html (AppVersion);

  // Check if this browser supports serial communication
  if (!('serial' in navigator) || navigator.serial == undefined)
    $('#smacPageArea').html ('<h1 style="color:#F00000; text-shadow:1px 1px 1px #000000; text-align:center">' +
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
    if (DateTimeInterval != undefined)
      clearInterval (DateTimeInterval);

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





      //===============================
      //  Go Fullscreen
      //===============================
      GoFullscreen ();





      window.devicePixelRatio = 1.0;  // set pixel density to 1

      // Load All Tabbed Pages
      LoadContentPages (() =>
      {
        // Trigger custom event if browser is resized.
        // This is so custom SMAC Widgets can listen for it and resize themselves.
        window.addEventListener ('resize', (event) => { $(document.body).trigger ('browserResized'); });

        //=================================================
        //  Bring Up Interface Screen
        //  Show nav buttons, diagnostics button and status bar
        //=================================================
        $('#navButtons'   ).css ('display', 'inline-block');
        $('#diagAndCBox'  ).css ('display', 'inline-block');
        $('#smacStatusBar').css ('display', 'flex'        );

        // Still booting up
        StatusBar.SetState ("Booting");

        UpdateConnectionBox ();

        // Update current date/time once per minute
        StatusBar.SetDateTime ();
        DateTimeInterval = setInterval (StatusBar.SetDateTime, 60000);

        // Click on the first nav button
        $('.navButton').first().trigger ('pointerdown');

        // Wait for Relayer
        setTimeout (async () =>
        {
          // Request full system info from the Relayer
          await Send_UItoRelayer (0, 0, 'SYSI');  // System Info command

          // Run 'UserStartup()' (defined by User at bottom of index.html)
          UserStartup ();

          // Running
          StatusBar.SetState   ("Running");
          StatusBar.SetMessage ("No Issues", "var(--light)");
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

//--- ProcessRelayerMessage -------------------------------

async function ProcessRelayerMessage (smacString)
{
  try
  {
    //=======================================================================
    // Get Node and Device Indexes for Data and Command Strings
    //=======================================================================
    if ((smacString[0] == 'D' || smacString[0] == 'C') && smacString[1] == '|')
    {
      // Data and Command strings start with NodeID and DeviceID:
      //
      //   ┌─────── 1-char packet type ('D' for Data, 'C' for Command)
      //   │ ┌───── 2-char source nodeID (00-19)
      //   │ │  ┌── 2-char source deviceID (00-99)
      //   │ │  │
      //   D|nn|dd|...

      // Split fields
      const fields = smacString.split ('|');
      if (fields.length < 4)
        throw "ProcessRelayerMessage(): Invalid Data or Command from Node/Device";

      const nodeIndex   = Number (fields[1]);
      const deviceIndex = Number (fields[2]);  // Could be NaN from "--" as deviceID

      // Must have a valid nodeID
      if (isNaN (nodeIndex))
        throw "ProcessRelayerMessage(): Invalid NodeID in Data or Commnand";

      // Set last message time
      if (Nodes[nodeIndex] != undefined)
        Nodes[nodeIndex].lastMsgTime = Date.now();

      // Log incoming message
      if (Diagnostics.DataLogging)
        Diagnostics.LogToMonitor (nodeIndex, '──▶ ' + smacString);


      //=======================================================================
      // Handle Data Strings first (for fast Widget updates)
      //=======================================================================
      if (smacString[0] == 'D')
      {
        const values    = fields[3];           // values should NOT have the '|' char in it !!!
        const timestamp = Number (fields[4]);

        // First, update Widgets with numeric device data values
        // They start with a dash or a digit
        if (!isNaN(deviceIndex) && (values[0] == '-' || (values[0] >= '0' && values[0] <= '9')))
        {
          // Update all UI Widgets with device data
          // These events are handled by SMAC Widgets
          $(document.body).trigger ('deviceData', [ nodeIndex, deviceIndex, values, timestamp ]);
          return;
        }

        //=====================================================================
        // Handle non-numeric Data values:
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
        //   ERROR:
        //   PONG
        //   FILES=
        //   FILE=
        //=====================================================================

        if (values.startsWith ('NOINFO='))
        {
          // Add new Node if it does NOT exist already
          if (Nodes[nodeIndex] == undefined)
          {
            // NOINFO=name,version,macAddress,numDevices
            // Update Node info fields: name, version, macAddress, numDevices
            const niFields   = values.split (',');
            Nodes[nodeIndex] = {
                                name        : niFields[0].substring(7),
                                version     : niFields[1],
                                macAddress  : niFields[2],
                                numDevices  : Number (niFields[3]),
                                devices     : [],          // Device objects { name, version, ipEnabled, ppEnabled, rate }
                                monitor     : undefined,
                                lastMsgTime : timestamp
                              };

            // Update the Diagnostics UI
            Diagnostics.BuildSystem ();

            // Always show this message
            Diagnostics.LogToMonitor (nodeIndex, 'Node ' + nodeIndex.toString() + ' connected.');



            // // Trigger an event to inform anyone that a new Node was added
            // $(document.body).trigger ('newNode', [ nodeIndex ]);


          }
        }

        else if (values.startsWith ('DEINFO='))
        {
          // Make sure Node exists in Diagnostics
          if (Nodes[nodeIndex] != undefined)
          {
            // One Device per 'DEINFO=...' message
            // DEINFO=name,version,ipEnabled,ppEnabled,rate
            const diFields = values.split (',');

            const deviceArray = Nodes[nodeIndex].devices;
            deviceArray[deviceIndex] = { name:diFields[0].substring(7), version:diFields[1], ipEnabled:diFields[2], ppEnabled:diFields[3], rate:diFields[4] };

            Diagnostics.UpdateDevices (nodeIndex);  // Update the UI Device fields of the Node Block in Diagnostics

            // Update status bar Device count
            let totalDevices = 0;
            Nodes.forEach ((node) =>
            {
              if (node != undefined)
                totalDevices += node.numDevices;
            });

            StatusBar.SetNumDevices (totalDevices);
          }
        }

        else if (values.startsWith ('NONAME='))
        {
          Nodes[nodeIndex].name = values.substring(7);
          $("#nodeField_name" + nodeIndex.toString()).html (Nodes[nodeIndex].name);
          $("#nmTab"          + nodeIndex.toString()).html (Nodes[nodeIndex].name);
        }

        else if (values.startsWith ('DENAME='))
        {
          Nodes[nodeIndex].devices[deviceIndex].name = values.substring(7);
          Diagnostics.UpdateDevices (nodeIndex);
        }

        else if (values.startsWith ('RATE='))
        {
          Nodes[nodeIndex].devices[deviceIndex].rate = values.substring(5);
          Diagnostics.UpdateDevices (nodeIndex);
        }

        else if (values == 'IP Enabled')
        {
          Nodes[nodeIndex].devices[deviceIndex].ipEnabled = 'Y';
          Diagnostics.UpdateDevices (nodeIndex);
        }

        else if (values == 'IP Disabled')
        {
          Nodes[nodeIndex].devices[deviceIndex].ipEnabled = 'N';
          Diagnostics.UpdateDevices (nodeIndex);
        }

        else if (values == 'PP Enabled')
        {
          Nodes[nodeIndex].devices[deviceIndex].ppEnabled = 'Y';
          Diagnostics.UpdateDevices (nodeIndex);
        }

        else if (values == 'PP Disabled')
        {
          Nodes[nodeIndex].devices[deviceIndex].ppEnabled = 'N';
          Diagnostics.UpdateDevices (nodeIndex);
        }

        else if (values == 'NVER=')
        {
          Nodes[nodeIndex].version = values.substring(5);
          Diagnostics.BuildSystem ();
        }

        else if (values == 'DVER=')
        {
          Nodes[nodeIndex].devices[deviceIndex].version = values.substring(5);
          Diagnostics.UpdateDevices (nodeIndex);
        }

        else if (values.startsWith ('ERROR:'))
        {
          // Error messages from a Node or Device post their error message in the values field.
          // The values field should start with ERROR:

          // Always show these messages
          Diagnostics.LogToMonitor (nodeIndex, values);
        }

        else if (values.startsWith ('PONG'))
        {
          // Alway show PONG messages
          Diagnostics.LogToMonitor (nodeIndex, 'PONG Received');
        }

        else if (values.startsWith ('FILES='))
        {
          // TODO: List of files
        }

        else if (values.startsWith ('FILE='))
        {
          // TODO: File contents
        }

        else
        {
          // Alway show unknow Data messages
          Diagnostics.LogToMonitor (nodeIndex, 'Unknown Data value: ' + values);
        }

      }  // end of Data String handling
    }


    //=======================================================================
    // Handle Special Messages
    //=======================================================================
    else if (smacString.startsWith ('NODE|'))
    {
      // New Node connected: NODE|nn
      const nodeID    = smacString.substring (5, 7);
      const nodeIndex = Number (nodeID);

      if (nodeIndex < MaxNodes)
      {
        PopupBar ('New Node connected to Relayer', 2000);

        // Always show this message
        Diagnostics.LogToMonitor (nodeIndex, '((( Node ' + nodeID + ' connected )))');

        // Request Node Info: D|nn|--|NOINFO=name,version,macAddress,numDevices|timestamp
        await Send_UItoRelayer (nodeIndex, 0, 'GNOI');

        // Request Device Info: D|nn|dd|DEINFO=name,version,ipEn(Y/N),ppEn(Y/N),rate|timestamp (for each Device)
        await Send_UItoRelayer (nodeIndex, 0, 'GDEI');
      }
    }

    // Show any Relayer/Node Error
    else if (smacString.startsWith ('ERROR:'))
    {
      // PopupMessage ('Relayer/Node Error', smacString.substring(6));
      // StatusBar.SetMessage ("Relayer/Node Error: " + smacString.substring(6), "#F00000");
      PopupBar ("Relayer/Node Error: " + smacString.substring(6), 2000);
    }

  }
  catch (ex)
  {
    ShowException (ex);
  }
}

//--- CheckNodes ------------------------------------------

function CheckNodes ()
{
  try
  {
    const now = Date.now ();

    // Check if all Nodes are still alive
    for (let i=0; i<MaxNodes; i++)
    {
      if (Nodes[i] != undefined)
      {
        if (now - Nodes[i].lastMsgTime > 31000)
        {


          // // Gray out Node box in Diagnostics
          // $('#diagSystemGroup....')



          StatusBar.SetMessage ("Node " + i.toString() + " not responding", "#F00000");
        }
        else
          StatusBar.SetMessage ("No issues");
      }
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
    // Command Format: C|nodeID|deviceID|command|params
    // where:
    //   type     = 'C' for Command
    //   nodeID   = 2-digits 00-19
    //   deviceID = 2-digits 00-99
    //   command  = 4-chars (usually caps)
    //   params   = optional parameters (null terminated string)

    if (nodeIndex == undefined || deviceIndex == undefined || commandString == undefined)
      return;

    const nodeID   = nodeIndex  .toString().padLeft ('0', 2);
    const deviceID = deviceIndex.toString().padLeft ('0', 2);
    const command  = commandString.substring (0, 4).padRight (' ', 4);
    const params   = (paramString == undefined || paramString == '') ? '' : '|' + paramString;

    const fullUIMessage = "C|" + nodeID + '|' + deviceID + '|' + command + params;

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
