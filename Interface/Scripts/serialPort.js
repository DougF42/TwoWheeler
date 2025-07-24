//=========================================================
//
//    FILE  : SerialPort.js
//
//  PROJECT : Any web project requiring Serial support
//
//     DOCS : https://web.dev/serial/ -> https://developer.chrome.com/articles/serial/
//            https://developer.mozilla.org/en-US/docs/Web/API/Web_Serial_API
//            https://developer.mozilla.org/en-US/docs/Web/API/SerialPort
//            https://developer.mozilla.org/en-US/docs/Web/API/Serial
//
//   AUTHOR : Bill Daniels
//            Copyright 2024-2025, D+S Tech Labs, Inc.
//            MIT License
//
//=========================================================


//--- class LineBreakTransformer --------------------------

class LineBreakTransformer
{
  constructor ()
  {
    this.container = '';
  }

  transform (chunk, controller)
  {
    this.container += chunk;
    const lines = this.container.split ('\r\n');
    this.container = lines.pop();
    lines.forEach (line => controller.enqueue(line));
  }

  flush (controller)
  {
    controller.enqueue (this.container);
  }
}


//=========================================================
//  class SerialPort
//=========================================================

class SerialPort
{
  constructor (inputProcessFunction, disconnectFunction)
  {
    // settings =
    // {
    //   baudRate    : 115200,
    //   dataBits    : 8,
    //   stopBits    : 1,
    //   parity      : 'none',
    //   flowControl : 'none'
    // }

    try
    {
      this.webPort            = undefined;
      this.serialReader       = undefined;
      this.serialWriter       = undefined;
      this.process            = inputProcessFunction;
      this.portOpened         = false;
      this.disconnectFunction = disconnectFunction;

      this.TDS = undefined;  // TextDecoderStream     - for reading
      this.TRS = undefined;  // TransformStream       - for reading
      this.RSC = undefined;  // ReadableStreamClosed  - for closing the read stream
      this.TES = undefined;  // TextEncoderStream     - for writing
      this.WSC = undefined;  // WriteableStreamClosed - for closing the writing stream

      // Check if this browser supports serial communication
      if (!('serial' in navigator) || navigator.serial == undefined)
        throw 'This browser does not support serial communications. Please use the Chrome or Edge browser.';
    }
    catch (ex)
    {
      alert ('SerialPort constructor(): ' + ex.message);
    }
  }

  //--- ChoosePort ----------------------------------------

  ChoosePort ()
  {
    return new Promise ((resolve, reject) =>
    {
      // Check if already open
      if (this.portOpened)
        reject (new Error ('Port is already open.'));

      //===============================
      //  Request Port
      //===============================
      navigator.serial.requestPort ()
      .then (port =>
      {
        // Check for valid port
        if (port == undefined || port == null)
          reject (new Error ('Invalid port from requestPort().'));

        this.webPort = port;

        resolve (this.webPort);
      })
      .catch (ex =>
      {
        reject (new Error (ex));  // No port choosen
      });
    });
  }

  //--- Open ----------------------------------------------

  Open (serialSettings)
  {
    return new Promise (async (resolve, reject) =>
    {
      //===============================
      //  Open Port
      //===============================
      await this.webPort.open (serialSettings)
      .then (async portOpen =>
      {
        // Check if port/device is valid
        const portInfo = this.webPort.getInfo ();
        if (portInfo == undefined)
          reject (new Error ('Unable to connect to serial device.'));
        // else if (portInfo.usbVendorId == undefined && portInfo.usbProductId == undefined)
        //   reject (new Error ('The serial device is not from a valid vendor.'));

        // Handle disconnect:
        // Call disconnect function specified in constructor
        this.webPort.addEventListener ('disconnect', async (event) =>
        {
          console.info ('Received disconnect event');

          await this.Close ();

          if (this.disconnectFunction != undefined)
            this.disconnectFunction ();
        });

        this.portOpened = true;
        resolve (true);
      })
      .catch (ex =>
      {
        console.info ('Open() failed: ' + ex);

        this.portOpened = false;
        reject (false);
      });
    });
  }

  //--- IsOpen --------------------------------------------

  IsOpen ()
  {
    return this.portOpened;
  }

  //--- ReadLoop ------------------------------------------

  async ReadLoop ()
  {
    try
    {
      let tryAgain = false;
      do
      {
        tryAgain = false;

        if (this.webPort.readable != undefined && this.webPort.readable != null)
        {
          //=================================================
          //  Setup Serial Reader for text
          //  Pipe input data thru a UTF-8 text decoder and line handler
          //=================================================
          this.TDS = new TextDecoderStream ();
          this.TRS = new TransformStream (new LineBreakTransformer ());
          this.RSC = this.webPort.readable.pipeThrough (this.TDS).pipeThrough (this.TRS);

          this.serialReader = this.RSC.getReader ();

          //=================================================
          //  Setup Serial Writer for text
          //=================================================
          this.TES = new TextEncoderStream ();
          this.WSC = this.TES.readable.pipeTo (this.webPort.writable);

          this.serialWriter = this.TES.writable.getWriter ();

          // Listen for incoming data
          while (true)
          {
            try
            {
              const { value, done } = await this.serialReader.read ();

              if (done)
                break;

              this.process (value);  // Process message from your device
            }
            catch (ex)
            {
              // Usually 'buffer overrun'
              await this.CloseStreams ();
              tryAgain = true;
              break;
            }
          }
        }
      }
      while (tryAgain);
    }
    catch (ex)
    {
      console.info (ex.message);
    }
  }

  //--- Send ----------------------------------------------

  async Send (data)
  {
    try
    {
      if (this.serialWriter != undefined && this.serialWriter != null)
        await this.serialWriter.write (data);
    }
    catch (ex)
    {
      alert ('Send(): ' + ex.message);
    }
  }

  //--- CloseStreams --------------------------------------

  async CloseStreams ()
  {
    try
    {
      if (this.serialReader != undefined && this.serialReader != null)
      {
        try { await this.serialReader.cancel (); } catch (ex) { }  // Ignore error if any
        await this.RSC;  // } catch (ex) { console.info ('Ignoring RSC error') }  // Ignore this error ???

        this.serialReader = undefined;
      }

      if (this.serialWriter != undefined && this.serialWriter != null)
      {
        try { await this.serialWriter.close (); } catch (ex) { }  // Ignore error if any
        await this.WSC;

        this.serialWriter = undefined;
      }
    }
    catch (ex)
    {
      alert ('CloseStreams(): ' + ex.message);
    }
  }

  //--- Close ---------------------------------------------

  async Close ()
  {
    try
    {
      await this.CloseStreams ();

      if (this.webPort != undefined && this.webPort != null)
      {
        try { await this.webPort.close (); } catch (ex) { }  // Ignore error if any

        this.webPort = undefined;
        this.portOpened = false;
      }
    }
    catch (ex)
    {
      alert ('Close(): ' + ex.message);
    }
  }

}  // End of class
