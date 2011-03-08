var timelineTimerID = null;

function getTimelineTheme() {
  var theme = Timeline.ClassicTheme.create();
  theme.event.label.width = 250; // px
  theme.event.bubble.width = 250;
  theme.event.bubble.height = 200;
  return theme;
}

function createTimelineWidget(id, orientation, initialDate, timeZone, units, url, feedType) {
  var eventSource = new Timeline.DefaultEventSource();
  var theme = getTimelineTheme();

  var bandInfos = [
    Timeline.createBandInfo({
      width:          '75%',
      intervalUnit:   units[0],
      intervalPixels: 200,
      eventSource:    eventSource,
      date:           initialDate,
      timeZone:       timeZone,
      theme:          theme
    }),
    Timeline.createBandInfo({
      width:          '25%',
      intervalUnit:   units[1],
      intervalPixels: 200,
      eventSource:    eventSource,
      date:           initialDate,
      timeZone:       timeZone,
      theme:          theme,
      showEventText:  false,
      trackHeight:    0.5,
      trackGap:       0.2
    })
  ];

  bandInfos[1].syncWith = 0;
  bandInfos[1].highlight = true;
  bandInfos[1].eventPainter.setLayout(bandInfos[0].eventPainter.getLayout());
  var timeline = Timeline.create(document.getElementById(id), bandInfos, orientation);

  if (feedType == 'json') {
    timeline.loadJSON(url, function(json, url) { eventSource.loadJSON(json, url); });
  }
  else {
    timeline.loadXML(url, function(xml, url) { eventSource.loadXML(xml, url); });
  }

  return timeline;
}

function centerTimeline(timeline, date) {
  timeline.getBand(0).setCenterVisibleDate(Timeline.DateTime.parseGregorianDateTime(date));
}

function setupTimelineControls(div, timeline, filterLabel, highlightLabel, clearButton) {
  var bandIndices = [0, 1];
  var theme = getTimelineTheme();

  var table = document.createElement('table');
  var tr = table.insertRow(0);
  var td = tr.insertCell(0);
  td.innerHTML = filterLabel;

  td = tr.insertCell(1);
  td.innerHTML = highlightLabel;

  var handler = function(elmt, evt, target) {
    if (timelineTimerID != null) {
      window.clearTimeout(timelineTimerID);
    }
    timelineTimerID = window.setTimeout(function() {
      performTimelineFiltering(timeline, bandIndices, table);
    }, 300);
  };

  tr = table.insertRow(1);
  tr.style.verticalAlign = 'top';

  td = tr.insertCell(0);

  var input = document.createElement('input');
  input.type = 'text';
  Timeline.DOM.registerEvent(input, 'keypress', handler);
  td.appendChild(input);

  for (var i = 0; i < theme.event.highlightColors.length; i++) {
    td = tr.insertCell(i + 1);

    input = document.createElement('input');
    input.type = 'text';
    Timeline.DOM.registerEvent(input, 'keypress', handler);
    td.appendChild(input);

    var divColor = document.createElement('div');
    divColor.style.height = '0.5em';
    divColor.style.background = theme.event.highlightColors[i];
    td.appendChild(divColor);
  }

  td = tr.insertCell(tr.cells.length);
  var button = document.createElement('button');
  button.innerHTML = clearButton;
  Timeline.DOM.registerEvent(button, 'click', function() {
    clearTimelineControls(timeline, bandIndices, table);
  });
  td.appendChild(button);

  document.getElementById(div).appendChild(table);
}

function cleanString(s) {
  return s.replace(/^\s+/, '').replace(/\s+$/, '');
}

function performTimelineFiltering(timeline, bandIndices, table) {
  timelineTimerID = null;

  var tr = table.rows[1];
  var text = cleanString(tr.cells[0].firstChild.value);

  var filterMatcher = null;
  if (text.length > 0) {
    var regex = new RegExp(text, 'i');
    filterMatcher = function(evt) {
      return regex.test(evt.getText()) || regex.test(evt.getDescription());
    };
  }

  var regexes = [];
  var hasHighlights = false;
  for (var x = 1; x < tr.cells.length - 1; x++) {
    var input = tr.cells[x].firstChild;
    var text2 = cleanString(input.value);
    if (text2.length > 0) {
      hasHighlights = true;
      regexes.push(new RegExp(text2, 'i'));
    }
    else {
      regexes.push(null);
    }
  }

  var highlightMatcher = hasHighlights ? function(evt) {
    var text = evt.getText();
    var description = evt.getDescription();
    for (var x = 0; x < regexes.length; x++) {
      var regex = regexes[x];
      if (regex != null && (regex.test(text) || regex.test(description))) {
        return x;
      }
    }
    return -1;
  } : null;

  for (var i = 0; i < bandIndices.length; i++) {
    var bandIndex = bandIndices[i];
    timeline.getBand(bandIndex).getEventPainter().setFilterMatcher(filterMatcher);
    timeline.getBand(bandIndex).getEventPainter().setHighlightMatcher(highlightMatcher);
  }
  timeline.paint();
}

function clearTimelineControls(timeline, bandIndices, table) {
  var tr = table.rows[1];
  for (var x = 0; x < tr.cells.length - 1; x++) {
    tr.cells[x].firstChild.value = '';
  }
  for (var i = 0; i < bandIndices.length; i++) {
    var bandIndex = bandIndices[i];
    timeline.getBand(bandIndex).getEventPainter().setFilterMatcher(null);
    timeline.getBand(bandIndex).getEventPainter().setHighlightMatcher(null);
  }
  timeline.paint();
}
