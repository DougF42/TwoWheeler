//=============================================================================
//
//     FILE : setMAC.js
//
//  PROJECT : SMAC
//
//  PURPOSE : This app connects to a Remote Module (Node) via serial port.
//            It will request the Relayer Module's MAC address and store it
//            in the Node's non-volatile memory.
//
//            - Load this file into the Chrome browser
//            - Connect the PC to the Node using a USB cable
//            - Follow on-screen instructions
//
//   AUTHOR : Bill Daniels
//            Copyright (c) 2022-2025, D+S Tech Labs, Inc.
//            All Rights Reserved
//
//=============================================================================

//--- Globals ---------------------------------------------

const AppVersion = '─── Version 2025.05.05 ───';

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

let RelayerMAC = undefined;

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
    // Create a SerialPort object
    SMACPort = new SerialPort (async (dataString) => { await ProcessModuleMessage (dataString); });  // do not include RelayerDisconnected param

    $('#connectToRelayer').show ();

    // Close serial port before exit
    window.addEventListener ('beforeunload', async (event) =>
    {
      StopEvent (event);
      event.returnValue = '';

      if (SMACPort != undefined)
        await SMACPort.Close ();

      return undefined;
    });
  }
}
catch (ex)
{
  ShowException (ex);
}

//--- ConnectToRelayer ------------------------------------

async function ConnectToRelayer ()
{
  try
  {
    await ConnectToRelayer_Async ();

    // Wait for Relayer to start
    setTimeout (async () =>
    {
      console.info ('Sending GMAC Request ...');

      // Send a request for the MAC address
      await SMACPort.Send ('--|--|GMAC' + EOL);
    }, 2000);
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
        PopupMessage ('Set MAC Tool', 'Unable to connect to Relayer.');
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

//--- ConnectToNode ---------------------------------------

async function ConnectToNode ()
{
  try
  {
    await ConnectToNode_Async ();

    // Wait for Node to start
    setTimeout (async () =>
    {
      console.info ('Request to set new MAC address ...');

      // Send a request to set the MAC address
      await SMACPort.Send ('SetRelayerMAC' + EOL);
    }, 2000);
  }
  catch (ex)
  {
    ShowException (ex);
  }
}

//--- ConnectToNode_Async ---------------------------------

async function ConnectToNode_Async ()
{
  try
  {
    if (SMACPort.IsOpen ());
      await SMACPort.Close ();

    // Select serial port
    await SMACPort.ChoosePort ()
    .then (async () =>
    {
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
        PopupMessage ('Set MAC Tool', 'Unable to connect to Node.');
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

//--- ProcessModuleMessage --------------------------------

async function ProcessModuleMessage (data)
{
  try
  {
    // Check if Relayer responded with MAC address
    if (data.startsWith ('MAC='))
    {
      RelayerMAC = data.substring(4).toUpperCase ();

      // Hide connection instructions
      $('#connectToRelayer').hide ();
      $('#connectToNode'   ).show ();

      $('#relayerMAC').html (RelayerMAC);

      // Done with Relayer
      await SMACPort.Close ();
    }

    // Check if Node responded with current MAC address
    else if (data.startsWith ('CurrentMAC='))
    {
      // Show current settings
      $('#connectToNode'  ).hide ();
      $('#keepOrSetNewMAC').show ();

      const currentMAC = data.substring (11).toUpperCase ();
      $('#currentMAC').html (currentMAC);

      // Show Relayer's MAC
      $("#newMAC").val (RelayerMAC);
    }

    // Check if successfully set MAC in Node
    else if (data.startsWith ('SetRelayerMAC-Success'))
    {
      $('#keepOrSetNewMAC').hide ();
      $('#newMACSet'      ).show ();
    }
  }
  catch (ex)
  {
    ShowException (ex);
  }
}

//--- SetRelayerMAC ---------------------------------------

async function SetRelayerMAC (event)
{
  try
  {
    const newMAC = $('#newMAC').val ();

    // Check input
    const regex = /^([0-9A-F]{2}[:]){5}([0-9A-F]{2})$/;
    if (newMAC.length != 17 || !regex.test (newMAC))
    {
      PopupBubble (event, 'Invalid MAC Address', 2000);
      console.info ('Invalid MAC Address.');
    }
    else
    {
      // Send new network credentials
      await SMACPort.Send ('NewMAC=' + newMAC + EOL);
    }
  }
  catch (ex)
  {
    ShowException (ex);
  }
}

//--- Reset -----------------------------------------------

function Reset ()
{
  try
  {
    $('#connectToRelayer').hide ();
    $('#keepOrSetNewMAC' ).hide ();
    $('#newMACSet'       ).hide ();
    $('#connectToNode'   ).show ();
  }
  catch (ex)
  {
    ShowException (ex);
  }
}
