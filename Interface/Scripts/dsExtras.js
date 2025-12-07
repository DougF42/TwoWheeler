//=========================================================
//
//     FILE : dsExtras.js
//
//  PROJECT : Common Web App Functions
//
//   AUTHOR : Bill Daniels
//            Copyright 2009-2025, D+S Tech Labs, Inc.
//            All Rights Reserved
//
//=========================================================

//---------------------------------------------------------
//  Globals
//---------------------------------------------------------

const dayName       = [ 'Sunday', 'Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday' ];
const dayNameAbbr   = [ 'Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat' ];
const monthName     = [ 'January', 'February', 'March', 'April', 'May', 'June', 'July', 'August', 'September', 'October', 'November', 'December' ];
const monthNameAbbr = [ 'Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec' ];

let dsDragCallback = undefined;

//---------------------------------------------------------
//  String Functions
//---------------------------------------------------------

if (typeof String.prototype.startsWith != 'function')
{
  String.prototype.startsWith = function (string)
  {
    return this.slice (0, string.length) == string;
  };
}

if (typeof String.prototype.endsWith != 'function')
{
  String.prototype.endsWith = function (string)
  {
    return this.slice (-string.length) == string;
  };
}

if (typeof String.prototype.padLeft != 'function')
{
  String.prototype.padLeft = function (padChar, totalLength)
  {
    let paddedString = this;

    while (paddedString.length < totalLength)
      paddedString = padChar + paddedString;

    return paddedString;
  };
}

if (typeof String.prototype.padRight != 'function')
{
  String.prototype.padRight = function (padChar, totalLength)
  {
    let paddedString = this;

    while (paddedString.length < totalLength)
      paddedString += padChar;

    return paddedString;
  };
}

if (typeof String.prototype.replaceAll != 'function')
{
  String.prototype.replaceAll = function (search, replace)
  {
    if (replace === undefined)
      return this.toString();

    return this.split (search).join (replace);
  }
}

if (typeof String.prototype.contains != 'function')
{
  String.prototype.contains = function (search)
  {
    return (this.indexOf (search) >= 0);
  }
}

//---------------------------------------------------------
//  Number/Date/Time Functions
//---------------------------------------------------------

//--- toFixedNoPad ----------------------------------------

if (typeof Number.prototype.toFixedNoPad != 'function')
{
  Number.prototype.toFixedNoPad = function (numDigits)
  {
    let numString = this.toFixed (numDigits);

    while (numString.endsWith ('0'))
      numString = numString.substring (0, numString.length-1);

    if (numString.endsWith ('.'))
      numString = numString.substring (0, numString.length-1);

    return numString;
  };
}

//--- Clamp -----------------------------------------------

function Clamp (value, minimum, maximum)
{
  try
  {
    return Math.min (Math.max (value, minimum), maximum);
  }
  catch (ex)
  {
    ShowException (ex);
  }

  return value;
}

//--- GetTimestamp ----------------------------------------

function GetTimestamp (newDate, format)
{
  // newDate is a Date object, usually from 'new Date()'
  // Timestamp starts with current hour:minutes, then format determines if seconds/milliseconds are added
  // Format is a string that contains: 24|s|m
  //                                    | | |
  //                                    | | +--- milliseconds
  //                                    | +----- seconds
  //                                    +------- 24-hour format

  try
  {
    // Return a current timestamp
    const hours = newDate.getHours();
    let   timestamp, am_pm = '';

    if (format.contains ('24'))
      timestamp = hours.toString().padLeft ('0', 2);
    else
    {
      if (hours > 12)
      {
        timestamp = (hours-12).toString();
        am_pm     = 'pm';
      }
      else
      {
        timestamp = hours.toString();
        am_pm     = 'am';
      }
    }

    timestamp += ':' + newDate.getMinutes().toString().padLeft  ('0', 2);

    if (format.contains ('s'))
      timestamp += ':' + newDate.getSeconds().toString().padLeft  ('0', 2);

    if (format.contains ('m'))
      timestamp += '.' + newDate.getMilliseconds().toString().padRight ('0', 3);

    return (timestamp + am_pm);
  }
  catch (ex)
  {
    ShowException (ex);
  }

  return '';
}

//--- Delay -----------------------------------------------
// Use with await

function Delay (ms)
{
  try
  {
    return new Promise (resolve => setTimeout (resolve, ms));
  }
  catch (ex)
  {
    ShowException (ex);
  }
}

//--- GetDayName ------------------------------------------

function GetDayName (newDate, abbr)
{
  try
  {
    // newDate is a Date object, usually from 'new Date()'
    // abbr is a flag to abbreviate or not
    const dayNumber = newDate.getDay ();
    return (abbr ? dayNameAbbr[dayNumber] : dayName[dayNumber]);
  }
  catch (ex)
  {
    ShowException (ex);
  }

  return '';
}

//--- GetMonthName ----------------------------------------

function GetMonthName (newDate, abbr)
{
  try
  {
    // newDate is a Date object, usually from 'new Date()'
    // abbr is a flag to abbreviate or not
    const monthNumber = newDate.getMonth ();
    return (abbr ? monthNameAbbr[monthNumber] : monthName[monthNumber]);
  }
  catch (ex)
  {
    ShowException (ex);
  }

  return '';
}

//--- GetBrowserWidth -----------------------------------

function GetBrowserWidth ()
{
  try
  {
    if (self.innerWidth)
      return self.innerWidth;
    if (document.documentElement && document.documentElement.clientWidth)
      return document.documentElement.clientWidth;
    if (document.body)
      return document.body.clientWidth;
  }
  catch (ex)
  {
    ShowException (ex);
  }

  return 0;
}

//--- GetBrowserHeight ----------------------------------

function GetBrowserHeight ()
{
  try
  {
    if (self.innerHeight)
      return self.innerHeight;
    if (document.documentElement && document.documentElement.clientHeight)
      return document.documentElement.clientHeight;
    if (document.body)
      return document.body.clientHeight;
  }
  catch (ex)
  {
    ShowException (ex);
  }

  return 0;
}

//--- GetClickSpecs ---------------------------------------

function GetClickSpecs (event)
{
  try
  {
    const rect   = event.target.getBoundingClientRect ();
    const mouseX = event.clientX - rect.left;
    const mouseY = event.clientY - rect.top;

    return { boundingRect:rect, x:mouseX, y:mouseY };
  }
  catch (ex)
  {
    ShowException (ex);
  }

  return undefined;
}

//--- RedrawElement ---------------------------------------

function RedrawElement (domElement)
{
  try
  {
    const orgDisplay = domElement.style.display;

    domElement.style.display = 'none';
    domElement.offsetHeight += 0;
    domElement.style.display = orgDisplay;
  }
  catch (ex)
  {
    ShowException (ex);
  }
}

//--- Event Propagation -----------------------------------

function StopEvent (event, preventDefault=true)
{
  try
  {
    if (event != undefined)
    {
      event.stopPropagation ();
      event.stopImmediatePropagation ();

      if (preventDefault)
        event.preventDefault ();

      if (window.event)
        window.event.cancelBubble = true;
    }
  }
  catch (ex)
  {
    ShowException (ex);
  }
}

//--- CloseKBOnEnter --------------------------------------

function CloseKBOnEnter (event)
{
  try
  {
    if (event != undefined)
    {
      StopEvent (event);
      if (event.keyCode == 13)
        event.target.blur();
    }
  }
  catch (ex)
  {
    ShowException (ex);
  }
}

//--- LoadContentPages ------------------------------------

function LoadContentPages (doneFunction)
{
  try
  {
    const contentPages = $('.contentPage').toArray ();
    const totalPages   = contentPages.length;

    // Load all pages recursively to keep them in order
    if (totalPages > 0)
    {
      let pageIndex = 0;
      loadPage ();

      function loadPage ()
      {
        if (pageIndex < totalPages)
        {
          const pageID = contentPages[pageIndex++].id;
          $('#' + pageID).load (pageID + '.html', loadPage);
        }
        else
        {
          // // Show first page
          // ShowPage (contentPages[0].id);

          // Call the 'done' function
          if (doneFunction != undefined)
            doneFunction ();
        }
      }
    }
  }
  catch (ex)
  {
    ShowException (ex);
  }
}

//--- ShowPage --------------------------------------------

function ShowPage (pageID, navButton)
{
  try
  {
    // Hide all pages
    $('.contentPage').hide ();

    // Remove 'viewing' class from all navButton's
    $('.navButton').removeClass ('viewing');

    // Show the specified page
    if (pageID != undefined)
      $('#' + pageID).show ();

    // Add 'viewing' class to this navButton
    if (navButton != undefined)
      navButton.classList.add ('viewing');
  }
  catch (ex)
  {
    ShowException (ex);
  }
}

//--- GoFullscreen ----------------------------------------

function GoFullscreen (nextHandler)
{
  try
  {
    // Go full screen
    // Must be called from a user event such as onclick
    // Chrome on Android has a bug if using onpointerdown
    document.body.requestFullscreen()
    .then (nextHandler)
    .catch (error =>
    {
      PopupMessage ('Error with requestFullscreen()', error);
    });
  }
  catch (ex)
  {
    ShowException (ex);
  }
}

//--- Modals ----------------------------------------------

function ShowModalHTML (htmlCode)
{
  // Popup any HTML code in a modal window
  // Example:
  //    <button onpointerdown='StopEvent(event); ShowModalHTML('<h1>Hello World!</h1>');'>
  //      Click me
  //    </button>

  try
  {
    // Clear any previous contents of the backdrop
    $('#modalBackdrop').empty ();

    // Add X to close
    $('#modalBackdrop').append ('<div class="modalX" onpointerdown="StopEvent(event); HideModal()">&#9447;</div>');

    // Add specified HTML content to a modal div
    $('#modalBackdrop').append ('<div class="modalDiv" onpointerdown="StopEvent(event)" onscroll="StopEvent(event)">' + htmlCode + '</div>');

    // Display the backdrop
    $('#modalBackdrop').css ('display', 'inline');
    $('#modalBackdrop').scrollTop (0);
  }
  catch (ex)
  {
    ShowException (ex);
  }
}

//--- ShowModalFile ---------------------------------------

function ShowModalFile (htmlFile)
{
  // Popup any HTML file in a modal window
  // Example:
  //    <button onpointerdown="StopEvent(event); ShowModalFile('hello.html');">
  //      Click me
  //    </button>

  try
  {
    // Clear any previous contents of the backdrop
    $('#modalBackdrop').empty ();

    // Add specified HTML content to a modal div
    $('#modalBackdrop').load (htmlFile, () =>
    {
      // Add X to close
      $('#modalBackdrop').append ('<div class="modalX" onpointerdown="StopEvent(event); HideModal()">&#9447;</div>');

      // Display the backdrop
      $('#modalBackdrop').css ('display', 'inline');
      $('#modalBackdrop').scrollTop (0);
    });
  }
  catch (ex)
  {
    ShowException (ex);
  }
}

//--- ShowModalImage --------------------------------------

function ShowModalImage (imageFile)
{
  try
  {
    // Clear any previous contents of the backdrop
    $('#modalBackdrop').empty ();

    // Add X to close
    $('#modalBackdrop').append ('<div class="modalX" onpointerdown="StopEvent(event); HideModal()">&#9447;</div>');

    // Add image to a modal div
    $('#modalBackdrop').append ('<div class="modalDiv" onpointerdown="StopEvent(event)"><img src="' + imageFile + '" /></div>');

    // Display the backdrop
    $('#modalBackdrop').css ('display', 'inline');
    $('#modalBackdrop').scrollTop (0);
  }
  catch (ex)
  {
    ShowException (ex);
  }
}

// --- ShowModalImageArray ---------------------------------
//
// function ShowModalImageArray (imageArray)
// {
//   try
//   {
//     let   picIndex     = 0;
//     const modaldivHTML = '<div class="modalDiv">' +
//                          '  <span style="font-size:100px; vertical-align:middle" onpointerdown="StopEvent(event); if (--picIndex < 0) picIndex=picArray.length-1;"><</span> ' +
//                          '  <img src="' + picArray[picIndex] + '" /> ' +
//                          '  <span style="font-size:100px; vertical-align:middle" onpointerdown="StopEvent(event); if (++picIndex > picArray.length) picIndex=0;">></span> ' +
//                          '</div>';
//
//     ShowModal (modalHTML);
//   }
//   catch (ex)
//   {
//     ShowException (ex);
//   }
// }

//--- HideModal -------------------------------------------

function HideModal ()
{
  try
  {
    $('#modalBackdrop').empty ();
    $('#modalBackdrop').css ('display', 'none');
  }
  catch (ex)
  {
    ShowException (ex);
  }
}

//--- ShowMenu --------------------------------------------

function ShowMenu ()
{
  try
  {
    $('#pageMenu').show ();
  }
  catch (ex)
  {
    ShowException (ex);
  }
}

//--- HideMenu --------------------------------------------

function HideMenu ()
{
  try
  {
    $('#pageMenu').hide ();
  }
  catch (ex)
  {
    ShowException (ex);
  }
}

//--- UpdateSlider ----------------------------------------

function UpdateSlider (rangeElement)
{
  try
  {
    // Fill track up to slider position
    const range = Number(rangeElement.max) - Number(rangeElement.min);
    if (range > 0)
    {
      const percent = Math.round (Number(rangeElement.value) * 100 / range).toString ();
      // rangeElement.style.setProperty ('--sliderProgress'    , 'linear-gradient(to right, var(--sliderFill) ' + percent + '%, transparent ' + percent + '%)');
      rangeElement.style.setProperty ('-webkit-slider-runnable-track', 'linear-gradient(to right, var(--sliderFill) ' + percent + '%, transparent ' + percent + '%)');
    }
  }
  catch (ex)
  {
    ShowException (ex);
  }
}

//--- SelectTab -------------------------------------------

function SelectTab (tabButton)
{
  try
  {
    const tabControl = $(tabButton).parent().parent();
    const tabIndex   = tabControl.children('.dsTabBar').children('.dsTabButton').index(tabButton);

    // Hide all tabContent div's
    tabControl.children ('.dsTabContent').hide ();

    // Check the tabButton
    $(tabButton).prop ('checked', true);

    // Show only the 'checked' tabButton's content div
    tabControl.children ('.dsTabContent').eq(tabIndex).show ();
  }
  catch (ex)
  {
    ShowException (ex);
  }
}

//--- PopupMessage ----------------------------------------

function PopupMessage (title, message)
{
  try
  {
    // !!! Be careful using element brackets <> in title or message
    //     It can cause a recursive exception!

    const popup = '<div class="dsGroupBox">' +
                  '<div class="dsGroupBoxTitle popupColors">' + title + '</div>' +
                  '<div class="dsGroupBoxContent" style="text-align:center">' + message + '<br><br>' +
                  '<button class="dsButton" onpointerdown="StopEvent(event); HideModal()">&nbsp; Okay &nbsp;</button><br>' +
                  '</div></div>';

    ShowModalHTML (popup);
  }
  catch (ex)
  {
    ShowException (ex);
  }
}

//--- PopupBubble -----------------------------------------

function PopupBubble (event, htmlMessage, fadeOut)
{
  try
  {
    // Remove any previous bubble
    RemoveBubble ();

    let x, y, popup;
    if (event == undefined)
    {
      x = (GetBrowserWidth ()/2                        ).toString() + 'px';
      y = (GetBrowserHeight()/2 + $(window).scrollTop()).toString() + 'px';

      // Add new bubble at center of screen
      popup = '<div id="screenBubble" class="popupColors" style="left:' + x + '; top:' + y + '; transform:translate(-50%, -50%)">' + htmlMessage;
    }
    else
    {
      x = event.clientX.toString() + 'px';
      y = (event.clientY + $(window).scrollTop()).toString() + 'px';

      // Add new bubble at click position
      popup = '<div id="screenBubble" class="popupColors" style="left:' + x + '; top:' + y + '">' + htmlMessage;
    }

    // Add [Close] button if not fading out
    if (fadeOut == undefined)
      popup += '<br><br><button class="dsButton" onpointerdown="StopEvent(event); RemoveBubble()"> Close </button>';

    popup += '</div>';

    // Show the popup
    $(popup).appendTo (document.body);

    // Remove bubble in <fadeOut> milliseconds
    if (fadeOut != undefined)
    {
      setTimeout (() =>
      {
        $('#screenBubble').fadeOut (1000, () => { RemoveBubble(); });
      }, fadeOut);
    }
  }
  catch (ex)
  {
    ShowException (ex);
  }
}

//--- PopupBar --------------------------------------------

function PopupBar (htmlMessage, timeout)
{
  try
  {
    // Remove any previous bubble
    RemoveBubble ();

    // Add new bubble
    const popup = '<div id="screenBubble" class="popupColors" style="position:fixed; left:0px; bottom:0px; width:100%">' + htmlMessage + '</div>';

    $(popup).appendTo (document.body);

    setTimeout (() =>
    {
      $('#screenBubble').fadeOut (1000, () => { RemoveBubble(); });
    }, timeout == undefined ? 2000 : timeout);
  }
  catch (ex)
  {
    ShowException (ex);
  }
}

//--- RemoveBubble ----------------------------------------

function RemoveBubble ()
{
  try
  {
    const bubble = $('#screenBubble');

    if (bubble != undefined)
      if (bubble.length)
        bubble.remove ();
  }
  catch (ex)
  {
    ShowException (ex);
  }
}

//--- Draggables ------------------------------------------

function BeginDrag (event, dragCallback)
{
  try
  {
    if (event != undefined)
    {
      StopEvent (event);

      event.target.style.cursor  = 'none';
      event.target.onpointermove = dragElement;
      event.target.onpointerup   = endDrag;
      event.target.setPointerCapture (event.pointerId);

      // Set global callback
      dsDragCallback = dragCallback;
    }
  }
  catch (ex)
  {
    ShowException (ex);
  }
}

function dragElement (event)
{
  try
  {
    StopEvent (event);

    const element      = event.target;
    const elementStyle = window.getComputedStyle (element);
    const parentStyle  = window.getComputedStyle (element.parentElement);

    const elementLeft   = parseInt (elementStyle.left  );
    const elementTop    = parseInt (elementStyle.top   );
    const elementWidth  = parseInt (elementStyle.width );
    const elementHeight = parseInt (elementStyle.height);
    const parentWidth   = parseInt (parentStyle .width );  // - parseInt (parentStyle.paddingLeft) - parseInt (parentStyle.paddingRight );
    const parentHeight  = parseInt (parentStyle .height);  // - parseInt (parentStyle.paddingTop ) - parseInt (parentStyle.paddingBottom);

    const maxLeft = parentWidth  - elementWidth;
    const maxTop  = parentHeight - elementHeight;

    const newLeft = Clamp (elementLeft + event.movementX, 0, maxLeft);
    const newTop  = Clamp (elementTop  + event.movementY, 0, maxTop );

    element.style.left = newLeft.toString() + 'px';
    element.style.top  = newTop .toString() + 'px';

    // Call drag callback if specified
    if (dsDragCallback != undefined)
      dsDragCallback (element);
  }
  catch (ex)
  {
    ShowException (ex);
  }
}

function endDrag (event)
{
  try
  {
    StopEvent (event);

    event.target.onpointermove = null;
    event.target.releasePointerCapture (event.pointerId);
    event.target.style.cursor = 'grab';

    dsDragCallback = undefined;
  }
  catch (ex)
  {
    ShowException (ex);
  }
}



//---------------------------------------------------------
//  Exception Handling
//---------------------------------------------------------

function ShowException (ex)
{
  // Show exception details
  try
  {
    let msg;
    if (ex.message == undefined)
      msg = ex.toString();
    else
      msg = ex.message;  // + '<br>' + ex.filename + ' (line ' + ex.lineNumber + ')';

    PopupMessage ('Application Error', msg);
  }
  catch (exSE)
  {
    alert ('Exception in ShowException():\n' + exSE.message);
  }
}
