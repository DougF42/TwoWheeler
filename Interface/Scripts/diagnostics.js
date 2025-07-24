//=========================================================
//
//     FILE : diagnostics.js
//
//  PROJECT : SMAC Interface
//
//   AUTHOR : Bill Daniels
//            Copyright 2014-2025, D+S Tech Labs, Inc.
//            All Rights Reserved
//
//=========================================================

const Diagnostics =
{
  DataLogging      : false,
  NumLogMessages   : 0,
  MaxLogMessages   : 500,    // Max number of log messages in a Node Monitor Box
  CurrentNodeIndex : 0,      // Currently selected Node Monitor Tab

  //--- BuildSystem----------------------------------------

  BuildSystem : function ()
  {
    try
    {
      // Clear and Re-Build Node System with Monitors
      $('#diagSystemGroup').children().slice(1).remove();
      $('#nmTabControl'   ).children().slice(1).remove();
      $('#nmTabBar'       ).empty ();

      let nodeCount = 0;
      for (let i=0; i<MaxNodes; i++)
      {
        if (Nodes[i] != undefined)
        {
          ++nodeCount;
          this.AddNode (i);
          this.UpdateDevices (i);

          // Set current node index to first Node found
          if (nodeCount == 1)
            this.CurrentNodeIndex = i;
        }
      }

      // Show/hide stuff
      if (nodeCount > 0)
      {
        $('#noNodes').hide ();
        $('#nodeMonitorGroup').css ('display', 'inline-block');

        // Select first tab
        const firstTab = document.getElementById ("nmTabBar").firstChild;
        SelectTab (firstTab);
      }
      else
      {
        $('#nodeMonitorGroup').hide ();
        $('#noNodes').show ();
      }

      // Update status bar Node count
      $("#statusBar_Nodes").html (nodeCount.toString());
    }
    catch (ex)
    {
      console.info ('Diagnostics.BuildSystem(): ' + ex.message);
      ShowException (ex);
    }
  },

  //--- AddNode -------------------------------------------

  AddNode : function (nodeIndex)
  {
    try
    {
      const nodeIndexString = nodeIndex.toString();

      // Create new Node Block
      const nodeBlock =
        '<div class="nodeBlock dsBlockLowered">' +
          '<span class="nodeIndex">' + nodeIndexString + '</span>' +  // + (nodeIndex > 9 ? "":"0")
          '<img class="nodeBox" src="Images/image_Node.png" />' +

          // Node Info, Buttons and Device List
          '<div class="nodeInfo">' +
            '<div style="display:inline-block">' +

              '<div class="dsGridForm" style="gap:0">' +
                '<label>         Node Name : </label><span id="nodeField_name'       + nodeIndexString + '">' + Nodes[nodeIndex].name                  + '</span>' +
                '<label>  Firmware Version : </label><span id="nodeField_version'    + nodeIndexString + '">' + Nodes[nodeIndex].version               + '</span>' +
                '<label>       MAC Address : </label><span id="nodeField_macAddress' + nodeIndexString + '">' + Nodes[nodeIndex].macAddress            + '</span>' +
                '<label> Number of Devices : </label><span id="nodeField_numDevices' + nodeIndexString + '">' + Nodes[nodeIndex].numDevices.toString() + '</span>' +
              '</div><hr>' +

              '<div class="flexRow" style="justify-content:space-around">' +
                '<div><button class="dsButton nodeButton"           title="Blink the Status LED" onclick="Send_UItoRelayer(' + nodeIndex + ', 0, \'BLIN\')"> Blink </button></div>' +
                '<div><button class="dsButton nodeButton nodePing"  title="Ping this Node"       onclick="Send_UItoRelayer(' + nodeIndex + ', 0, \'PING\')"> Ping  </button></div>' +
                '<div><button class="dsButton nodeButton nodeReset" title="Reset this Node"      onclick="Send_UItoRelayer(' + nodeIndex + ', 0, \'RSET\')"> Reset </button></div>' +
              '</div>' +

            '</div><br>' +

            // Device List
            '<div id="deviceInfo' + nodeIndexString + '" class="deviceInfo dsGridForm" style="gap:0"></div>' +

          '</div>' +
        '</div><br><br>'  // end of nodeBlock

      // Insert Node Block to the SMAC System Group
      $('#diagSystemGroup').append (nodeBlock);

      // Add a Data Monitor
      const tBox = $('#nmTabControl');
      const tBar = $('#nmTabBar');

      // Add a tab for the new monitor box
      const tbID = 'nmTab' + nodeIndexString;

      const tabButton = '<input type="radio" class="dsTabButton" id="' + tbID + '" name="nmTabControl" '
                      + 'onclick="SelectTab(this); Diagnostics.CurrentNodeIndex=' + nodeIndexString + ';" />';

      const tabButtonLabel = '<label for="' + tbID + '">Node ' + nodeIndexString + '</label>';  // .padLeft ('0', 2)

      tBar.append (tabButton);
      tBar.append (tabButtonLabel);

      // Add a tab content with a monitor box
      const newTabContent = $('<div class="dsTabContent" style="padding:0"></div>');

      // const cols       = Math.floor (GetBrowserWidth () / 16).toString ();
      // const rows       = Math.floor (GetBrowserHeight() / 27).toString ();
      // const newMonitor = $('<textarea class="monitorBox dsScrollable" cols="' + cols + '" rows="' + rows + '" readonly></textarea>');
      const newMonitor = $('<textarea class="monitorBox dsScrollable" readonly></textarea>');

      newTabContent.append (newMonitor);
      tBox.append (newTabContent);

      // Save monitor in the Node object
      Nodes[nodeIndex].monitor = newMonitor;
    }
    catch (ex)
    {
      console.info ('AddNode() exception: ' + ex.message);
      ShowException (ex);
    }
  },

  //--- UpdateDevices -------------------------------------

  UpdateDevices : function (nodeIndex)
  {
    try
    {
      const nodeIndexString = nodeIndex.toString();
      const deviceArray     = Nodes[nodeIndex].devices;

      // Clear current device list
      $('#deviceInfo' + nodeIndexString).empty();
      $('#deviceInfo' + nodeIndexString).append ('<div><b>▐<br>▐     ─── Device List ─── </b></div><div> </div>');

      for (let devIndex=0; devIndex<deviceArray.length; devIndex++)
      {
        const devIndexString = devIndex.toString();
        const sph            = deviceArray[devIndex].rate;
        const sps            = Math.round (sph/360) / 10;

        $('#deviceInfo' + nodeIndexString).append ('<div>▐══ ' + (devIndex > 9 ? '':'0') + devIndexString + ': ' + deviceArray[devIndex].name + ' </div>');
        $('#deviceInfo' + nodeIndexString).append ('<div>ip:<input type="checkbox" class="dsSwitch" title="Turn On/Off Immediate Processing" ' + (deviceArray[devIndex].ipEnabled=='Y' ? 'checked':'') + ' onchange="Diagnostics.ToggleImmediate(this, ' + nodeIndexString + ', ' + devIndexString + ')" />   ' +
                                                        'pp:<input type="checkbox" class="dsSwitch" title="Turn On/Off Periodic Processing" '  + (deviceArray[devIndex].ppEnabled=='Y' ? 'checked':'') + ' onchange="Diagnostics.TogglePeriodic (this, ' + nodeIndexString + ', ' + devIndexString + ')" />   ' +
                                                   '<input type="range" class="dsInput" style="width:6vw; font-size:0.6vw" min="1" max="72000" step="1" value="' + sph.toString() + '" title="Set rate of Periodic Processing\nRate = ' + sph + ' samples per hour\n        ~' + sps.toString() +
                                                   ' samples per sec" onpointerup="Diagnostics.SetPPRate (this, ' + nodeIndexString + ', ' + devIndexString + ')" /> &nbsp;' + deviceArray[devIndex].version + '</div>');



//         //  Buttons for [DoImmediate] and [DoPeriodic]
//         // $('#deviceOnOff'      + nodeIndexString).append ('<input type="number" class="dsInput" style="width:3vw; font-size:.6vw" min="0" max="72000" step="3600" value="3600" title="Rate of Periodic Processing: (samples per hour)\nClick, then use ↑↓ arrow keys to change." onkeydown="Diagnostics.SetPPRate (this, ' + nodeIndex + ', ' + devIndex + ')" /><br>');
//         // $('#deviceInfoValues' + nodeIndexString).append ('Rate = ' + deviceArray[devIndex].rate.toString() + ' samples/hour<br>');
//         //                                                  <input type="number" class="dsInput" style="width:4vw; font-size:medium" min="1" step="1" value="' + deviceArray[devIndex].rate.toString() + '" />


      }
    }
    catch (ex)
    {
      ShowException (ex);
    }
  },

  //--- ToggleImmediate -----------------------------------

  ToggleImmediate : async function (slideSwitch, nodeIndex, devIndex)
  {
    try
    {
      if (slideSwitch != undefined)
        await Send_UItoRelayer (nodeIndex, devIndex, (slideSwitch.checked ? 'ENIP' : 'DIIP'));
    }
    catch (ex)
    {
      ShowException (ex);
    }
  },

  //--- TogglePeriodic ------------------------------------

  TogglePeriodic : async function (slideSwitch, nodeIndex, devIndex)
  {
    try
    {
      if (slideSwitch != undefined)
        await Send_UItoRelayer (nodeIndex, devIndex, (slideSwitch.checked ? 'ENPP' : 'DIPP'));
    }
    catch (ex)
    {
      ShowException (ex);
    }
  },

  //--- SetPPRate -----------------------------------------

  SetPPRate : async function (slider, nodeIndex, devIndex)
  {
    try
    {
      if (slider != undefined)
      {
        // Don't allow a zero rate
        if (parseInt (slider.value) < 1)
          slider.value = '1';

        await Send_UItoRelayer (nodeIndex, devIndex, 'SRAT', slider.value);
      }
    }
    catch (ex)
    {
      ShowException (ex);
    }
  },

  //--- LogToMonitor --------------------------------------

  LogToMonitor : function (nodeIndex, logMessage)
  {
    try
    {
      if (Nodes[nodeIndex] != undefined)
      {
        // Output message to appropriate monitor
        const mBox = Nodes[nodeIndex].monitor;

        // Add to monitor's <textarea>
        if (this.NumLogMessages < Diagnostics.MaxLogMessages)
        {
          mBox.val (mBox.val() + logMessage + '\n');
          ++this.NumLogMessages;
        }
        else
        {
          mBox.val (logMessage + '\n');
          this.NumLogMessages = 1;
        }

        if ($('#autoScroll').is (':checked'))
          mBox.scrollTop (1E10);
      }
    }
    catch (ex)
    {
      console.info ('LogToMonitor() exception: ' + ex.message);
      ShowException (ex);
    }
  },

  //--- ClearMonitor --------------------------------------

  ClearMonitor : function (clearAll)
  {
    try
    {
      if (clearAll)
      {
        Nodes.forEach ((node) =>
        {
          if (node.monitor != undefined)
            // node.monitor.empty ();
            node.monitor.val ('');
        });
      }
      else
      {
        const node = Nodes[this.CurrentNodeIndex];
        if (node != undefined)
          // node.monitor.empty ();
          node.monitor.val ('');
      }
    }
    catch (ex)
    {
      ShowException (ex);
    }
  },

  //--- UserCommandBox_keyup --------------------------------

  UserCommandBox_keyup : async function (keyEvent)
  {
    try
    {
      // Check for [Enter] key
      // Send command to currently selected Node (tab)
      if (keyEvent.which === 13)
      {
        const commandString = $('#UserCommandBox').val();
        const commandParams = commandString.split ('|');

        await Send_UItoRelayer (Diagnostics.CurrentNodeIndex, parseInt (commandParams[0]), commandParams[1], commandParams[2]);
      }
    }
    catch (ex)
    {
      ShowException (ex);
    }
  }

};
