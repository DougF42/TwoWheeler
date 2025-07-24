//=============================================================================
//
//     FILE  : WidgetTemplate.js
//
//   PROJECT : System Monitor and Control (SMAC)
//
//   AUTHOR  : Bill Daniels
//             Copyright 2020-2025, D+S Tech Labs, Inc.
//             All Rights Reserved
//
//=============================================================================


//=============================================================================
//  Control Widget (Sends commands to Devices)
//=============================================================================

class SMAC_Control extends HTMLElement
{
  //--- Constructor ---------------------------------------

  constructor ()
  {
    super ();

    // Rebuild this widget on resize
    $(document.body).on ('browserResized', () => { this.build (); });

    // Right-click context menu
    this.addEventListener ('contextmenu', function (event)
    {
      StopEvent (event);

      // ...
    });
  }

  //--- Attributes ----------------------------------------

  get attrib1 (     ) { return this.Attrib1;  }  // Required attribute
  set attrib1 (value) { this.Attrib1 = value; }

  get attrib2 (     ) { return this.Attrib2;  }  // Optional attribute
  set attrib2 (value) { this.Attrib2 = value; }

  //--- connectedCallback ---------------------------------

  connectedCallback ()
  {
    try
    {
      // Check and set required attributes
      if (!this.hasAttribute ('attrib1'))
        throw '(smac-widget): Missing attrib1 attribute';
      this.Attrib1 = this.getAttribute ('attrib1');

      // Set optional attributes
      this.Attrib2 = this.hasAttribute ('attrib2') ? this.getAttribute ('attrib2') : undefined;

      SetAsInlineBlock (this);

      // Build this widget
      this.build ();
    }
    catch (ex)
    {
      ShowException (ex);
    }
  }

  //--- build ---------------------------------------------

  build = function ()
  {
    try
    {
      // If a graphic canvas element is needed ...
      // Canvas dimensions are percentages of browser's size
      const cWidth  = Math.max (Math.round (this.Width  * GetBrowserWidth () / 100), 62);
      const cHeight = Math.max (Math.round (this.Height * GetBrowserHeight() / 100), 30);

      // Create a 2D data visualization canvas (dvCanvas2D)
      this.smacCanvas = new dvCanvas2D (cWidth, cHeight, 'transparent');

      // Check if a canvas already exists from previous build
      if (this.firstChild == undefined)
        this.appendChild  (this.smacCanvas.canvas);
      else
        this.replaceChild (this.smacCanvas.canvas, this.firstChild);  // Replace existing canvas


      // :
      // :
      // :


    }
    catch (ex)
    {
      ShowException (ex);
    }
  }
}

customElements.define ('smac-control', SMAC_Control);


//=============================================================================
//  Data View Widget (Reacts to Device data)
//=============================================================================

class SMAC_DataView extends HTMLElement
{
  //--- Constructor ---------------------------------------

  constructor ()
  {
    super ();

    // Rebuild this widget on resize
    $(document.body).on ('browserResized', () => { this.build (); });

    // Right-click context menu
    this.addEventListener ('contextmenu', function (event)
    {
      StopEvent (event);

      // ...
    });
  }

  //--- Attributes ----------------------------------------

  get device  (     ) { return this.Device;   }  // required attribute
  set device  (value) { this.Device = value;  }

  get attrib1 (     ) { return this.Attrib1;  }  // Required attribute
  set attrib1 (value) { this.Attrib1 = value; }

  get attrib2 (     ) { return this.Attrib2;  }  // Optional attribute
  set attrib2 (value) { this.Attrib2 = value; }

  //--- connectedCallback ---------------------------------

  connectedCallback ()
  {
    try
    {
      // Check and set required attributes
      if (!this.hasAttribute ('device'))
        throw '(smac-growbar): Missing device attribute';
      const deviceInfo = this.getAttribute ('device').replaceAll (' ', '').split (',');
      this.NodeID   = parseInt (deviceInfo[0]);
      this.DeviceID = parseInt (deviceInfo[1]);

      if (!this.hasAttribute ('attrib1'))
        throw '(smac-widget): Missing attrib1 attribute';
      this.Attrib1 = this.getAttribute ('attrib1');

      // Set optional attributes
      this.Attrib2 = this.hasAttribute ('attrib2') ? this.getAttribute ('attrib2') : undefined;

      SetAsInlineBlock (this);

      // Build this widget
      this.build ();

      //--- React to device data ---
      const self = this;
      $(document.body).on ('deviceData', function (event, nodeID, deviceID, timestamp, value)
      {
        // Update this widget if its NodeID and DeviceID match
        if (nodeID == self.NodeID && deviceID == self.DeviceID)
          window.requestAnimationFrame.bind (self.updateWidget (Number(value)));
      });
    }
    catch (ex)
    {
      ShowException (ex);
    }
  }

  //--- build ---------------------------------------------

  build = function ()
  {
    try
    {
      // Canvas dimensions are percentages of browser's size
      const cWidth  = Math.max (Math.round (this.Width  * GetBrowserWidth () / 100), 62);
      const cHeight = Math.max (Math.round (this.Height * GetBrowserHeight() / 100), 30);

      // Create a 2D data visualization canvas (dvCanvas2D)
      this.smacCanvas = new dvCanvas2D (cWidth, cHeight, 'transparent');

      // Check if a canvas already exists from previous build
      if (this.firstChild == undefined)
        this.appendChild  (this.smacCanvas.canvas);
      else
        this.replaceChild (this.smacCanvas.canvas, this.firstChild);  // Replace existing canvas

      // Set initial bar size
      this.BarWidth  = cWidth;
      this.BarHeight = cHeight;
      this.OffsetX   = 0;
      this.OffsetY   = 0;

      // Reduce bar area and adjust offsets for scale, if specified
      if (this.ScaleColor != undefined)
      {
        if (this.Horizontal)
        {
          this.BarWidth  -= 50;
          this.BarHeight -= 28;

          this.OffsetX = 25;
          if (this.ScalePlacement == 'top')
            this.OffsetY = 28;
        }
        else  // vertical
        {
          this.BarWidth  -= 60;
          this.BarHeight -= 10;

          this.OffsetY = 5;
          if (this.ScalePlacement == 'left')
            this.OffsetX = 60;
        }
      }

      // Back and Fill Gradients
      if (this.Horizontal)
      {
        this.BackGrad = this.smacCanvas.cc.createLinearGradient (this.OffsetX, this.OffsetY, this.OffsetX+this.BarWidth, this.OffsetY);
        this.BackGrad.addColorStop (0.0, this.smacCanvas.adjustBrightness (this.BackColor, -30));  // darker
        this.BackGrad.addColorStop (1.0, this.smacCanvas.adjustBrightness (this.BackColor,  30));  // lighter

        this.FillGrad = this.smacCanvas.cc.createLinearGradient (this.OffsetX, this.OffsetY, this.OffsetX+this.BarWidth, this.OffsetY);
        this.FillGrad.addColorStop (0.0, this.smacCanvas.adjustBrightness (this.FillColor, -30));  // darker
        this.FillGrad.addColorStop (1.0, this.smacCanvas.adjustBrightness (this.FillColor,  30));  // lighter
      }
      else
      {
        this.BackGrad = this.smacCanvas.cc.createLinearGradient (this.OffsetX, this.OffsetY, this.OffsetX, this.OffsetY+this.BarHeight);
        this.BackGrad.addColorStop (0.0, this.smacCanvas.adjustBrightness (this.BackColor,  30));  // lighter
        this.BackGrad.addColorStop (1.0, this.smacCanvas.adjustBrightness (this.BackColor, -30));  // darker

        this.FillGrad = this.smacCanvas.cc.createLinearGradient (this.OffsetX, this.OffsetY, this.OffsetX, this.OffsetY+this.BarHeight);
        this.FillGrad.addColorStop (0.0, this.smacCanvas.adjustBrightness (this.FillColor,  30));  // lighter
        this.FillGrad.addColorStop (1.0, this.smacCanvas.adjustBrightness (this.FillColor, -30));  // darker
      }

      // Clear bar area
      this.smacCanvas.drawRectangle (this.OffsetX, this.OffsetY, this.BarWidth, this.BarHeight, this.BackGrad, fill);

      // Manually calculate scale factor if scaleColor not specified
      if (this.ScaleColor == undefined)
        this.ScaleFactor = (this.Horizontal ? this.BarWidth : this.BarHeight) / (this.MaxValue - this.MinValue);
      else
      {
        // drawLinearScale() returns the scale factor for you
        if (this.Horizontal)
        {
          if (this.ScalePlacement == 'top')
            this.ScaleFactor = this.smacCanvas.drawLinearScale (this.OffsetX, this.OffsetY  , this.BarWidth, this.BarHeight, ScaleOrientation.HorizTop   , this.MinValue, this.MaxValue, this.Units, this.ScaleColor);
          else  // bottom
            this.ScaleFactor = this.smacCanvas.drawLinearScale (this.OffsetX, this.BarHeight, this.BarWidth, this.BarHeight, ScaleOrientation.HorizBottom, this.MinValue, this.MaxValue, this.Units, this.ScaleColor);
        }
        else  // vertical
        {
          if (this.ScalePlacement == 'right')
            this.ScaleFactor = this.smacCanvas.drawLinearScale (this.BarWidth, this.BarHeight+this.OffsetY, this.BarWidth, this.BarHeight, ScaleOrientation.VertRight, this.MinValue, this.MaxValue, this.Units, this.ScaleColor);
          else  // left
            this.ScaleFactor = this.smacCanvas.drawLinearScale (this.OffsetX , this.BarHeight+this.OffsetY, this.BarWidth, this.BarHeight, ScaleOrientation.VertLeft , this.MinValue, this.MaxValue, this.Units, this.ScaleColor);
        }
      }

      // Set clipping region
      this.smacCanvas.setClipRectangle (this.OffsetX, this.OffsetY, this.BarWidth, this.BarHeight);
    }
    catch (ex)
    {
      ShowException (ex);
    }
  }

  //--- updateWidget --------------------------------------

  updateWidget = function (value)
  {
    try
    {
      let level = Math.round ((value - this.MinValue) * this.ScaleFactor);
      if (level < 0) level = 0;

      this.smacCanvas.drawRectangle (this.OffsetX, this.OffsetY, this.BarWidth, this.BarHeight, this.BackGrad, fill);

      if (this.Horizontal)
        this.smacCanvas.drawRectangle (this.OffsetX, this.OffsetY, level, this.BarHeight, this.FillGrad, fill);
      else
        this.smacCanvas.drawRectangle (this.OffsetX, this.OffsetY + this.BarHeight - level, this.BarWidth, level, this.FillGrad, fill);

      // Check alarm ranges
      if ((this.AlarmLow  != undefined && value <= this.AlarmLow ) ||
          (this.AlarmHigh != undefined && value >= this.AlarmHigh))
        this.style.border = '0.5vw solid #FF0000';
      else
        this.style.border = this.OrgBorder;
    }
    catch (ex)
    {
      ShowException (ex);
    }
  }
}

customElements.define ('smac-dataview', SMAC_DataView);
