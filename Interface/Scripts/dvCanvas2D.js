//=============================================================================
//
//     FILE  : dvCanvas2D.js
//
//   PROJECT : Any browser-based project
//
//  FUNCTION : Wrapper methods for HTML5 Canvas Context (2D)
//             Plus some Data Visualization methods
//
//     NOTES : This class creates the <canvas> element for you.
//             ∙ The canvas element performs better when its height
//               is a power of 2 (height = 16, 32, 64, 128, 256, 512, 1024, ...
//             ∙ Do not use style to set canvas dimensions (causes blurry dithered plots).
//             ∙ Full Spec   : https://html.spec.whatwg.org/multipage/canvas.html
//             ∙ Performance : https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Optimizing_canvas
//             ∙ Compositing : https://developer.mozilla.org/en-US/docs/Web/API/CanvasRenderingContext2D/globalCompositeOperation
//
//   AUTHOR  : Bill Daniels
//             Copyright 2020-2024, D+S Tech Labs, Inc.
//             All Rights Reserved
//
//=============================================================================


//--- Globals -----------------------------------------------------------------

const fill        = 0;
const pi          = Math.PI;
const twopi       = 2 * pi;
const piOver2     = pi / 2;
const piOver4     = pi / 4;
const deg2rad     = pi / 180;
const percent2rad = twopi / 100;

//--- Scale Orientations ---
const ScaleOrientation = Object.freeze
({
	HorizTop    : Symbol('1'),
	HorizBottom : Symbol('2'),
	VertLeft    : Symbol('3'),
	VertRight   : Symbol('4')
})


//=============================================================================
//  dvCanvas2D class (Data Visualization Canvas)
//=============================================================================

class dvCanvas2D
{
  //--- Public properties ---------------------------------
  cWidth       = 1;          // canvas width
  cHeight      = 1;          // canvas height
  backColor    = '#000000';  // clearing color
  canvas       = undefined;  // <canvas> element
  cc           = undefined;  // canvas context 2D
  canvasRect   = undefined;  // bounding client rect of canvas
  backImage    = undefined;  // background of draggable (full image of canvas)
  dragImage    = undefined;  // image of draggable
  dragX        = 0;          // current draggable location
  dragY        = 0;          // current draggable location

  //--- Constructor ---------------------------------------
  constructor (width, height, backColor)
  {
    try
    {
      // Create a <canvas> element
      this.canvas = document.createElement ('canvas');

      // Get the canvas context
      this.cc = this.canvas.getContext ('2d', { alpha:true, willReadFrequently:true });  // transparent and performant

      // Set specified size
      this.resize (width, height);

      this.backColor = backColor == undefined ? 'black' : backColor;
      this.canvas.style.userSelect = 'none';

      // Reset the context to default settings
      this.reset ();
    }
    catch (ex)
    {
      ShowException (ex);
    }
  }

  //--- reset ---------------------------------------------
  reset ()
  {
    try
    {
      this.cc.reset();

      // No scaling
      // Also fix canvas's half pixel origin: shift for integer performance
      this.cc.setTransform (1, 0, 0, 1, 0.5, 0.5);

      this.cc.globalAlpha           = 1.0;     // fully opaque
      this.cc.lineWidth             = 1;       // thin lines
      this.cc.lineCap               = 'butt';  // quicker lines
      this.cc.shadowBlur            = 0;       // no shadow blur
      this.cc.filter                = 'none';  // no filtering
      this.cc.imageSmoothingEnabled = false;   // no smoothing
      this.cc.originClean           = true;    // images with opaque content allowed

      // Default colors
      this.cc.strokeStyle = 'black';
      this.cc.fillStyle   = 'black';

      // Default font settings
      this.cc.font          = '12px sans-serif';
      this.cc.textAlign     = 'left';
      this.cc.textBaseline  = 'top';
      this.cc.textRendering = 'optimizeSpeed';

      // Clear canvas and shadow
      this.clear ();
      this.setShadow (0, 0, 'transparent', 0);
    }
    catch (ex)
    {
      ShowException (ex);
    }
  }

  //--- resize --------------------------------------------
  resize (newWidth, newHeight)
  {
    try
    {
      // Integer size only
      newWidth  = Math.floor (newWidth );
      newHeight = Math.floor (newHeight);

      // Must be non-zero
      if (newWidth > 0 && newHeight > 0)
      {
        // Set new property values for this object and the canvas
        this.cWidth  = newWidth;
        this.cHeight = newHeight;

        this.canvas.width  = newWidth;
        this.canvas.height = newHeight;

        this.canvas.setAttribute ('width' , (this.cWidth ).toString() + 'px');
        this.canvas.setAttribute ('height', (this.cHeight).toString() + 'px');
      }
    }
    catch (ex)
    {
      ShowException (ex);
    }
  }

  //--- clear ---------------------------------------------
  clear ()
  {
    try
    {
      this.cc.fillStyle = this.backColor;
      this.cc.fillRect (0, 0, this.cWidth, this.cHeight);
    }
    catch (ex)
    {
      ShowException (ex);
    }
  }

  //--- setClipRectangle ----------------------------------
  setClipRectangle (x, y, width, height)
  {
    try
    {
      this.cc.beginPath ();
      this.cc.rect (x, y, width, height);
      this.cc.clip ();
    }
    catch (ex)
    {
      ShowException (ex);
    }
  }

  //--- adjustBrightness (lighten/darken %)----------------
  adjustBrightness (color, amount)
  {
    try
    {
      return '#' + color.replace(/^#/, '').replace(/../g, color =>
        ('0' + Math.min(255, Math.max(0, parseInt(color, 16) + amount)).toString(16)).substr(-2));
    }
    catch (ex)
    {
      ShowException (ex);
    }

    return color;
  }

  //--- setShadow -----------------------------------------
  setShadow (xOffset, yOffset, color, blurDistance)
  {
    try
    {
      if (xOffset      != undefined) this.cc.shadowOffsetX = xOffset;
      if (yOffset      != undefined) this.cc.shadowOffsetY = yOffset;
      if (color        != undefined) this.cc.shadowColor   = color;
      if (blurDistance != undefined) this.cc.shadowBlur    = blurDistance;
    }
    catch (ex)
    {
      ShowException (ex);
    }
  }

  //--- drawPixel -----------------------------------------
  drawPixel (x, y, color)
  {
    try
    {
      this.cc.fillStyle = color;
      this.cc.fillRect (x, y, 1, 1);
    }
    catch (ex)
    {
      ShowException (ex);
    }
  }

  //--- drawPoint -----------------------------------------
  drawPoint (x, y, radius, color, thickness=1)
  {
    try
    {
      this.cc.beginPath ();
      this.cc.arc (x, y, radius, 0, twopi);

      if (thickness == fill)
      {
        this.cc.fillStyle = color;
        this.cc.fill ();
      }
      else
      {
        this.cc.strokeStyle = color;
        this.cc.lineWidth = thickness;
        this.cc.stroke ();
      }
    }
    catch (ex)
    {
      ShowException (ex);
    }
  }

  //--- drawLine ------------------------------------------
  drawLine (x1, y1, x2, y2, color, thickness=1, endCap='butt')
  {
    try
    {
      this.cc.strokeStyle = color;
      this.cc.lineWidth = thickness;
      this.cc.lineCap = endCap;  // 'butt' | 'round' | 'square'
      this.cc.beginPath ();
      this.cc.moveTo (x1, y1);
      this.cc.lineTo (x2, y2);
      this.cc.stroke ();
    }
    catch (ex)
    {
      ShowException (ex);
    }
  }

  //--- drawRectangle -------------------------------------
  drawRectangle (x, y, width, height, color, thickness=1)
  {
    try
    {
      if (thickness == fill)
      {
        this.cc.fillStyle = color;
        this.cc.fillRect (x, y, width-1, height-1);
      }
      else
      {
        this.cc.strokeStyle = color;
        this.cc.lineWidth = thickness;
        this.cc.beginPath ();
        this.cc.rect (x, y, width-1, height-1);
        this.cc.stroke ();
      }
    }
    catch (ex)
    {
      ShowException (ex);
    }
  }

  //--- drawPoly ------------------------------------------
  drawPoly (vertexArray, color, thickness=1, joint='miter', close=true)
  {
    try
    {
      this.cc.beginPath ();
      this.cc.moveTo (vertexArray[0].x, vertexArray[0].y);
      for (let i=1; i<vertexArray.length; i++)
        this.cc.lineTo (vertexArray[i].x, vertexArray[i].y);
      if (close) this.cc.closePath ();

      this.cc.lineJoin = joint;

      if (thickness == fill)
      {
        this.cc.fillStyle = color;
        this.cc.fill ();
      }
      else
      {
        this.cc.strokeStyle = color;
        this.cc.lineWidth = thickness;
        this.cc.stroke ();
      }
    }
    catch (ex)
    {
      ShowException (ex);
    }
  }

  //--- drawEllipse ---------------------------------------
  drawEllipse (cx, cy, radX, radY, color, thickness=1, angle=0)
  {
    // angle:  0° is straight up (12 o'clock)
    //       +90° is clockwise to East (3 o'clock)
    //       -90° is counter-clockwise West (9 o'clock)
    //       180° is straight down (6 o'clock)

    try
    {
      this.cc.beginPath ();
      this.cc.ellipse (cx, cy, radX, radY, angle*deg2rad, 0, twopi);

      if (thickness == fill)
      {
        this.cc.fillStyle = color;
        this.cc.fill ();
      }
      else
      {
        this.cc.strokeStyle = color;
        this.cc.lineWidth = thickness;
        this.cc.stroke ();
      }
    }
    catch (ex)
    {
      ShowException (ex);
    }
  }

  //--- drawArc -------------------------------------------
  drawArc (cx, cy, radius, color, startAngle, stopAngle, thickness=1)
  {
    // angle:  0° is straight up (12 o'clock)
    //       +90° is clockwise to East (3 o'clock)
    //       -90° is counter-clockwise West (9 o'clock)
    //       180° is straight down (6 o'clock)

    try
    {
      while (stopAngle < startAngle)
        stopAngle += 360;

      if (startAngle == stopAngle)
        return;

      this.cc.beginPath ();
      this.cc.arc (cx, cy, radius, startAngle*deg2rad-piOver2, stopAngle*deg2rad-piOver2);

      if (thickness == fill)
      {
        this.cc.fillStyle = color;
        this.cc.fill ();
      }
      else
      {
        this.cc.strokeStyle = color;
        this.cc.lineWidth = thickness;
        this.cc.stroke ();
      }
    }
    catch (ex)
    {
      ShowException (ex);
    }
  }

  //--- drawCurve -----------------------------------------
  drawCurve (x1, y1, x2, y2, color, hv='h', thickness=1, endCap='butt')
  {
    try
    {
      this.cc.strokeStyle = color;
      this.cc.lineWidth = thickness;
      this.cc.lineCap = endCap;  // 'butt' | 'round' | 'square'

      // Must be drawn left to right for horizontally weighted curves
      //  --OR--
      // top to bottom for vertically weighted curves
      if (((hv=='h') && (x1 > x2)) || ((hv=='v') && (y1 > y2)))
      {
        let temp = x1;
        x1 = x2;
        x2 = temp;

        temp = y1;
        y1 = y2;
        y2 = temp;
      }

      let diffO3, cp1X, cp1Y, cp2X, cp2Y;

      if (hv == 'v')
      {
        diffO3 = (y2 - y1) / 3;
        cp1X   = x1;
        cp1Y   = y1 + diffO3;
        cp2X   = x2;
        cp2Y   = y2 - diffO3;
      }
      else
      {
        diffO3 = (x2 - x1) / 3;
        cp1X   = x1 + diffO3;
        cp1Y   = y1;
        cp2X   = x2 - diffO3;
        cp2Y   = y2;
      }

      this.cc.beginPath ();
      this.cc.moveTo (x1, y1);
      this.cc.bezierCurveTo (cp1X, cp1Y, cp2X, cp2Y,    x2, y2);
      this.cc.stroke ();
    }
    catch (ex)
    {
      ShowException (ex);
    }
  }

  //--- drawPie -------------------------------------------
  drawPie (cx, cy, radius, color, startAngle, stopAngle)
  {
    // angle:  0° is straight up (12 o'clock)
    //       +90° is clockwise to East (3 o'clock)
    //       -90° is counter-clockwise to West (9 o'clock)
    //       180° is straight down (6 o'clock)

    try
    {
      while (stopAngle < startAngle)
        stopAngle += 360;

      if (startAngle == stopAngle)
        return;

      startAngle = startAngle * deg2rad - piOver2;
      stopAngle  = stopAngle  * deg2rad - piOver2;

      this.cc.beginPath ();
      this.cc.arc (cx, cy, radius, startAngle, stopAngle);
      this.cc.lineTo (cx, cy);
      this.cc.lineTo (cx + Math.cos(startAngle), cy + Math.sin(startAngle));

      this.cc.fillStyle = color;
      this.cc.fill ();
    }
    catch (ex)
    {
      ShowException (ex);
    }
  }

  //--- getTextSize ---------------------------------------
  getTextSize (text, font)
  {
    try
    {
      if (font != undefined)
        this.cc.font = font;

      const tm = this.cc.measureText (text);

      const tmSize =
      {
        width  : Math.round (tm.actualBoundingBoxRight  - tm.actualBoundingBoxLeft),
        height : Math.round (tm.actualBoundingBoxAscent + tm.actualBoundingBoxDescent)
      };

      return tmSize;
    }
    catch (ex)
    {
      ShowException (ex);
    }

    return 0;
  }

  //--- drawText ------------------------------------------
  drawText (x, y, text, color, font)
  {
    try
    {
      if (color != undefined)
        this.cc.fillStyle = color;

      if (font != undefined)
        this.cc.font = font;

      this.cc.textBaseline = 'top';

      this.cc.fillText (text, x, y);
    }
    catch (ex)
    {
      ShowException (ex);
    }
  }

  //--- getBlock ------------------------------------------
  getBlock (x, y, width, height)
  {
    // May generate a security error, check for null return
    // Consult 'Access-Control-Allow-Origin' HTTP Header
    try
    {
      return this.cc.getImageData (x, y, width, height);
    }
    catch (ex)
    {
      ShowException (ex);
    }

    return null;
  }

  //--- drawBlock -----------------------------------------
  drawBlock (x, y, block)
  {
    try
    {
      // NOTE: drawImage is much faster than putImageData

      this.cc.putImageData (block, x, y);
    }
    catch (ex)
    {
      ShowException (ex);
    }
  }

  //--- drawImage -----------------------------------------
  drawImage (x, y, url, loadedCallback)
  {
    try
    {
      // url cannot be file:///....
      // Must load from local or remote server to avoid CORS block
      const img = new Image();
      img.crossOrigin = '*';
      img.onload = () =>
      {
        this.cc.drawImage (img, x-1, y-1);

        if (loadedCallback != undefined)
          loadedCallback (img);
      };

      // Start loading
      img.src = url;
    }
    catch (ex)
    {
      ShowException (ex);
    }
  }

  //--- drawDraggable -------------------------------------
  drawDraggable (x, y, url, dragCallback, doneCallback, allowH=true, allowV=true)  // allow both horizontal and vertical motion by default
  {
    try
    {
      // Capture entire canvas as background image
      this.backImage = new Image ();
      this.backImage.src = this.canvas.toDataURL ();

      // Draw draggable image
      this.drawImage (x, y, url, (img) =>
      {
        // Save returned image as draggable
        this.dragImage = img;
        this.dragX = x;
        this.dragY = y;

        // Check for mouse down
        this.downListener = this.canvas.onpointerdown = (event) =>
        {
          // Get location of canvas (in case browser scrolled)
          this.canvasRect = this.canvas.getBoundingClientRect ();

          // Get location of cursor when clicked (relative to canvas)
          const cX = event.clientX - this.canvasRect.x;
          const cY = event.clientY - this.canvasRect.y;

          // Check if on draggable image
          if (cX >= this.dragX && cX < this.dragX + this.dragImage.width &&
              cY >= this.dragY && cY < this.dragY + this.dragImage.height)
          {
            // Remove move listener on mouse up and mouse going off canvas
            this.canvas.onpointerup  = // () => { this.canvas.onpointermove = null; }
            this.canvas.onpointerout = () =>
            {
              this.canvas.onpointermove = null;
              document.body.style.cursor = 'default';

              // Call callback function, if any
              if (doneCallback != undefined)
                doneCallback (this.dragX, this.dragY);
            }

            // Limit motion to within canvas (0..max)
            const maxX = this.canvasRect.width  - this.dragImage.width;
            const maxY = this.canvasRect.height - this.dragImage.height;

            let newX, newY;

            // Hide cursor
            document.body.style.cursor = 'none';

            // Begin dragging
            this.moveListener = this.canvas.onpointermove = (event) =>
            {
              // Get new location of image
              newX = allowH ? this.dragX + event.movementX : x;
              newY = allowV ? this.dragY + event.movementY : y;

              // Check bounds
              if (newX < 0) newX = 0; else if (newX > maxX) newX = maxX;
              if (newY < 0) newY = 0; else if (newY > maxY) newY = maxY;

              // Set new location of draggable
              this.dragX = newX;
              this.dragY = newY;

              // Replace background
              this.cc.drawImage (this.backImage, -1, -1);  // the whole canvas seems to shift +1 pixel both horiz and vert !!!

              // Call callback function, if any
              if (dragCallback != undefined)
                dragCallback (newX, newY);

              // Draw draggable at new location
              this.cc.drawImage (this.dragImage, newX, newY);
            }
          }
        };
      });
    }
    catch (ex)
    {
      ShowException (ex);
    }
  }

  //--- drawLinearScale -----------------------------------
  drawLinearScale (x, y, width, height, orientation, minValue, maxValue, unit, scaleColor, gridColor)
  {
    //-------------------------------------------------
    //  - This method can draw a linear scale in four orientations.
    //    Set orientation to one of:
    //      ScaleOrientation.HorizTop
    //      ScaleOrientation.HorizBottom
    //      ScaleOrientation.VertLeft
    //      ScaleOrientation.VertRight
    //
    //  - Major scale values step by 1,2 or 5 * 10^n
    //    (1,2,5 or 10,20,50 or 100,200,500 etc.)
    //
    //  - 1/10th and 1/2 tick marks are drawn between values
    //
    //  - Scale text and ticks are drawn if scaleColor is specified
    //
    //  - Grid lines are drawn if gridColor is specified
    //
    //  - This function returns the scaleFactor to convert real value to pixel.
    //    Use the following to get the pixel location on this scale:
    //      px = x + Math.round ((value - minValue) * scaleFactor);
    //-------------------------------------------------

    try
    {
      // Orientation abbreviation
      let o = 'vr';
      switch (orientation)
      {
        case ScaleOrientation.HorizTop    : o = 'ht'; break;
        case ScaleOrientation.HorizBottom : o = 'hb'; break;
        case ScaleOrientation.VertLeft    : o = 'vl'; break;
      //case ScaleOrientation.VertRight   : o = 'vr'; break;
      }

      // Set font for scale text
      this.cc.font = '12px sans-serif';

      // Determine real value to pixel scale factor and major/minor steps for tick marks
      const scaleFactor       = (o.startsWith('h') ? width : height) / (maxValue - minValue);
      const majorStepLog      = Math.log10 ((o.startsWith('h') ? 80.0 : 65.0) / scaleFactor);  // 80px horizontal -OR- 65px vertical
      const majorStepPower    = Math.floor (majorStepLog);                                     // spacing between major steps
      const majorStepFraction = majorStepLog - majorStepPower;
      const textHeight        = this.getTextSize ('80').height + 2;

      let majorStep = Math.pow (10, majorStepPower);
      let vFactor   = 1;
           if (majorStepFraction > 0.85) vFactor = 10;
      else if (majorStepFraction > 0.5 ) vFactor =  5;
      else if (majorStepFraction > 0.15) vFactor =  2;
      majorStep *= vFactor;

      const minorStep = majorStep / 10;  // step by 1/10ths

      // Determine where to start tick marks (some trickery here)
      const vStart = minorStep * Math.ceil (minValue / minorStep);
      let v = vStart, i = 10;
      while (Math.abs(v % majorStep) > 0.0001)  // is not near zero
      {
        v += minorStep;
        --i;
      }

      // Draw major, half-major and minor tick marks
      // Also draw major step value text and grid lines (if gridColor specified)
      let   value, cx, cy, scaleText, textOffset, unitDrawn=false;
      const fudge = 0.00001;  // fudge factor to deal with floating point in-exactness

      if (o.startsWith('h'))
      {
        //-----------------------------------------
        //  Horizontal scale (Top or Bottom)
        //-----------------------------------------
        const tickLength = height / 4.0;
        const tl1 = Clamp (Math.round (tickLength    ), 6, 15);
        const tl2 = Clamp (Math.round (tickLength/1.5), 4, 12);
        const tl3 = Clamp (Math.round (tickLength/3.0), 2,  6);

        const cy = y;
        for (value=vStart; value<maxValue+fudge; value+=minorStep, ++i)
        {
          cx = x + Math.round ((value - minValue) * scaleFactor);

          // Is major step?
          if (Math.abs(i % 10) < fudge)  // near zero?
          {
            if (scaleColor != undefined)
            {
              // Full tick
              this.drawLine (cx, cy, cx, o.endsWith('b') ? cy+tl1 : cy-tl1, scaleColor);

              // Value text
              scaleText = value.toFixedNoPad(4); if (scaleText == '-0') scaleText = '0';
              textOffset = Math.floor (this.getTextSize(scaleText).width / 2);
              if (!unitDrawn) { scaleText += unit; unitDrawn = true; }
              this.drawText (cx-textOffset, o.endsWith('b') ? cy+tl1 : cy-tl1-textHeight, scaleText, scaleColor);
            }

            // Grid line?
            if (gridColor != undefined)
              this.drawLine (cx, cy, cx, o.endsWith('b') ? cy-height : cy+height, gridColor);
          }
          // else is half major step?
          else if ((Math.abs(i % 5) < fudge) && (scaleColor != undefined))  // near zero?
            // Medium tick (1/2)
            this.drawLine (cx, cy, cx, o.endsWith('b') ? cy+tl2 : cy-tl2, scaleColor);
          // else it's a minor step
          else if (scaleColor != undefined)
            // Small tick (1/10)
            this.drawLine (cx, cy, cx, o.endsWith('b') ? cy+tl3 : cy-tl3, scaleColor);
        }
      }
      else
      {
        //-----------------------------------------
        //  Vertical scale (Left or Right)
        //-----------------------------------------
        const tickLength = width / 4.0;
        const tl1 = Clamp (Math.round (tickLength    ), 6, 18);
        const tl2 = Clamp (Math.round (tickLength/1.5), 4, 13);
        const tl3 = Clamp (Math.round (tickLength/3.0), 2,  8);

        const cx = x;
        for (value=vStart; value<maxValue+fudge; value+=minorStep, ++i)
        {
          cy = y - Math.round ((value - minValue) * scaleFactor);

          if (Math.abs(i % 10) < fudge)  // near zero?
          {
            if (scaleColor != undefined)
            {
              // Full tick
              this.drawLine (cx, cy, o.endsWith('r') ? cx+tl1 : cx-tl1, cy, scaleColor);

              // Value text
              scaleText = value.toFixedNoPad(4); if (scaleText == '-0') scaleText = '0';
              if (!unitDrawn) { scaleText += unit; unitDrawn = true; }
              this.drawText (o.endsWith('r') ? cx+tl1+1 : cx-this.getTextSize(scaleText).width-tl1-1, cy-textHeight/2-1, scaleText, scaleColor);
            }

            // Grid line?
            if (gridColor != undefined)
              this.drawLine (cx, cy, o.endsWith('r') ? cx-width : cx+width, cy, gridColor);
          }
          // else is half major step?
          else if ((Math.abs(i % 5) < fudge) && (scaleColor != undefined))  // near zero?
            // Medium tick
            this.drawLine (cx, cy, o.endsWith('r') ? cx+tl2 : cx-tl2, cy, scaleColor);
          // else it's a minor step
          else if (scaleColor != undefined)
            // Small tick
            this.drawLine (cx, cy, o.endsWith('r') ? cx+tl3 : cx-tl3, cy, scaleColor);
        }
      }

      return scaleFactor;
    }
    catch (ex)
    {
      ShowException (ex);
    }

    return 1;
  }

  //--- drawLogScale --------------------------------------
  drawLogScale (x, y, width, height, orientation, minValue, maxValue, unit, scaleColor, gridColor)
  {
    // See notes for drawLinearScale (above)
    try
    {
      // Orientation abbreviation
      let o = 'vr';
      switch (orientation)
      {
        case ScaleOrientation.HorizTop    : o = 'ht'; break;
        case ScaleOrientation.HorizBottom : o = 'hb'; break;
        case ScaleOrientation.VertLeft    : o = 'vl'; break;
      //case ScaleOrientation.VertRight   : o = 'vr'; break;
      }

      // Set font for scale text
      this.cc.font = '12px sans-serif';

      if (minValue < 0.001) minValue = 0.001;
      const minValueLog = Math.log10 (minValue);

      // Determine real value to pixel scale factor
      const scaleFactor = (o.startsWith('h') ? width : height) / (Math.log10(maxValue) - minValueLog);

      // Determine where to start tick marks
      let vPower = Math.floor (minValueLog);
      let vStep  = Math.ceil (minValue / (10 ** vPower));
      let value  = vStep * (10 ** vPower);

      // Draw major, half-major and minor tick marks
      // Also draw major step value text and grid lines (if gridColor specified)
      let   cx, cy, scaleText, textOffset, unitDrawn=false;
      const textHeight = this.getTextSize ('80').height + 2;

      const fudge = 0.00001;  // fudge factor to deal with floating point in-exactness

      if (o.startsWith('h'))
      {
        //-----------------------------------------
        //  Horizontal scale (Up or Down)
        //-----------------------------------------
        const tickLength = height / 4.0;
        const tl1 = Clamp (Math.round (tickLength    ), 6, 15);
        const tl2 = Clamp (Math.round (tickLength/1.5), 4, 12);

        const cy = y;
        while (value < maxValue+fudge)
        {
          cx = x + Math.round ((Math.log10(value) - minValueLog) * scaleFactor);

          // Is full step?
          if ((vStep - Math.floor(vStep)) < fudge)  // near zero?
          {
            if (scaleColor != undefined)
            {
              // Full tick
              this.drawLine (cx, cy, cx, o.endsWith('b') ? cy+tl1 : cy-tl1, scaleColor);

              // Value Text?
              //if ((vStep - 1) < fudge || (vStep % 2) < fudge)  // 1 or even?
              if (vStep==0 || vStep==1 || vStep==2 || vStep==3 || vStep==5)
              {
                // Value text
                if (vPower < 3)
                  scaleText = value.toFixedNoPad(3);
                else if (vPower < 6)
                  scaleText = (value / 10**3).toFixedNoPad(3) + 'K';  // Kilo
                else if (vPower < 9)
                  scaleText = (value / 10**6).toFixedNoPad(3) + 'M';  // Mega
                else if (vPower < 12)
                  scaleText = (value / 10**9).toFixedNoPad(3) + 'G';  // Giga

                textOffset = Math.floor (this.getTextSize(scaleText).width / 2);
                if (!unitDrawn) { scaleText += unit; unitDrawn = true; }
                this.drawText (cx-textOffset, o.endsWith('b') ? cy+tl1 : cy-tl1-textHeight, scaleText, scaleColor);
              }
            }

            // Grid line?
            if (gridColor != undefined)
              this.drawLine (cx, cy, cx, o.endsWith('b') ? cy-height : cy+height, gridColor);
          }
          else if (scaleColor != undefined)
            // Half step
            this.drawLine (cx, cy, cx, o.endsWith('b') ? cy+tl2 : cy-tl2, scaleColor);

          // Increment value step by 1/2
          vStep += 0.5;
          if (vStep > 9.6)
          {
            vStep = 1;
            ++vPower;
          }

          // Calc new value
          value = vStep * (10 ** vPower);
        }
      }
      else
      {
        //-----------------------------------------
        //  Vertical scale (Left or Right)
        //-----------------------------------------
        const tickLength = width / 4.0;
        const tl1 = Clamp (Math.round (tickLength    ), 6, 18);
        const tl2 = Clamp (Math.round (tickLength/1.5), 4, 13);

        const cx = x;
        while (value < maxValue+fudge)
        {
          cy = y - Math.round ((Math.log10(value) - minValueLog) * scaleFactor);

          // Is full step?
          if ((vStep - Math.floor(vStep)) < fudge)  // near zero?
          {
            if (scaleColor != undefined)
            {
              // Full tick
              this.drawLine (cx, cy, o.endsWith('r') ? cx+tl1 : cx-tl1, cy, scaleColor);

              // Value Text?
              //if ((vStep - 1) < fudge || (vStep % 2) < fudge)  // 1 or even?
              if (vStep==0 || vStep==1 || vStep==2 || vStep==3 || vStep==5)
              {
                // Value text
                if (vPower < 3)
                  scaleText = value.toFixedNoPad(3);
                else if (vPower < 6)
                  scaleText = (value / 10**3).toFixedNoPad(3) + 'K';  // Kilo
                else if (vPower < 9)
                  scaleText = (value / 10**6).toFixedNoPad(3) + 'M';  // Mega
                else if (vPower < 12)
                  scaleText = (value / 10**9).toFixedNoPad(3) + 'G';  // Giga

                if (!unitDrawn) { scaleText += unit; unitDrawn = true; }
                this.drawText (o.endsWith('r') ? cx+tl1+1 : cx-this.getTextSize(scaleText).width-tl1+1, cy-textHeight/2-1, scaleText, scaleColor);
              }
            }

            // Grid line?
            if (gridColor != undefined)
              this.drawLine (cx, cy, o.endsWith('r') ? cx-width : cx+width, cy, gridColor);
          }
          else if (scaleColor != undefined)
            // Half step
            this.drawLine (cx, cy, o.endsWith('r') ? cx+tl2 : cx-tl2, cy, scaleColor);

          // Increment value step by 1/2
          vStep += 0.5;
          if (vStep > 9.6)
          {
            vStep = 1;
            ++vPower;
          }

          // Calc new value
          value = vStep * (10 ** vPower);
        }
      }
    }
    catch (ex)
    {
      ShowException (ex);
    }
  }

  //--- drawCircularScale ---------------------------------
  drawCircularScale (cx, cy, radius, minValue, maxValue, startAngle, stopAngle, unit, scaleColor)
  {
    // angle:  0° is straight up (12 o'clock)
    //       +90° is clockwise to East (3 o'clock)
    //       -90° is counter-clockwise West (9 o'clock)
    //       180° is straight down (6 o'clock)
    //
    // This function returns the object { startRad:<value>, scaleFactor:<value> } to convert real value to radian angle.
    // Use the following to get a value angle on this scale:
    //
    //   needleAngle = cScaleReturn.startRad + (value - minValue) * cScaleReturn.scaleFactor;
    //

    try
    {
      while (stopAngle < startAngle)
        stopAngle += 360;

      if (startAngle == stopAngle)
        return;

      // //--- For debugging -----------------------------------------------
      // this.drawPie (cx, cy, radius, randColor(), startAngle, stopAngle);
      // //-----------------------------------------------------------------

      const startRad    = (startAngle - 90) * deg2rad;
      const stopRad     = (stopAngle  - 90) * deg2rad;
      const scaleFactor = (stopRad - startRad) / (maxValue - minValue);

      // Set font for scale text
      this.cc.font = '12px sans-serif';

      // Scale values step by 1,2,5 * 10^n : 1,2,5 or 10,20,50 or 100,200,500 etc.
      const angleStepLog      = Math.log10 (40 / radius / scaleFactor);  // step depends on radius
      const angleStepPower    = Math.floor (angleStepLog);
      const angleStepFraction = angleStepLog - angleStepPower;
      let   angleStep         = Math.pow (10, angleStepPower);

           if (angleStepFraction > 0.85) angleStep *= 10;
      else if (angleStepFraction > 0.5 ) angleStep *=  5;
      else if (angleStepFraction > 0.15) angleStep *=  2;

      let value, tickAngle, x1, y1, x2, y2, sinTA, cosTA, scaleText, textSize, tx, ty, unitDrawn=false;

      const minorStep = angleStep / 4;  // step by 1/4ths

      // Determine where to start tick marks (some trickery here)
      const vStart = minorStep * Math.ceil (minValue / minorStep);
      let v = vStart, i = 10;
      while (Math.abs(v % angleStep) > 0.0001)  // is not near zero
      {
        v += minorStep;
        --i;
      }

      const tickLength = radius / 8.0;
      const tl1 = Clamp (Math.round (tickLength    ), 6, 20);
      const tl2 = Clamp (Math.round (tickLength/1.5), 4, 13);
      const tl3 = Clamp (Math.round (tickLength/3.0), 2,  6);

      for (value=vStart; value<=maxValue; value+=minorStep, i++)
      {
        tickAngle = startRad + (value - minValue) * scaleFactor;
        sinTA = Math.sin (tickAngle);
        cosTA = Math.cos (tickAngle);

        x1 = cx + radius * cosTA;
        y1 = cy + radius * sinTA;

        if (i % 4 == 0)
        {
          // Long tick with text
          scaleText = value.toFixedNoPad (3);
          textSize  = this.getTextSize (scaleText);
          x2        = cx + (radius + tl1) * cosTA;
          y2        = cy + (radius + tl1) * sinTA;
          tx        = cx + (radius + tl1 + 15) * cosTA;
          ty        = cy + (radius + tl1 + 15) * sinTA;

          // Add unit text, if specified
          if (unit != undefined && !unitDrawn)
          {
            scaleText += ' ' + unit;
            unitDrawn = true;
          }

          this.drawText (tx - textSize.width/2, ty - textSize.height/2, scaleText, scaleColor);
          this.drawLine (x1, y1, x2, y2, scaleColor);
        }
        else if (i % 4 == 2)
        {
          // Medium tick
          x2 = cx + (radius + tl2) * cosTA;
          y2 = cy + (radius + tl2) * sinTA;
          this.drawLine (x1, y1, x2, y2, scaleColor);
        }
        else
        {
          // Short tick
          x2 = cx + (radius + tl3) * cosTA;
          y2 = cy + (radius + tl3) * sinTA;
          this.drawLine (x1, y1, x2, y2, scaleColor);
        }
      }

      return { startRad : startRad, scaleFactor : scaleFactor };
    }
    catch (ex)
    {
      ShowException (ex);
    }
  }

}  // class dvCanvas2D
