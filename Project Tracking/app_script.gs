function doGet(e) {
  const token = e.parameter.token;
  const correctToken = PropertiesService.getScriptProperties().getProperty("API_TOKEN");

  if (token !== correctToken) {
    return ContentService
      .createTextOutput(JSON.stringify({ error: "Unauthorized" }))
      .setMimeType(ContentService.MimeType.JSON);
  }
  const sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName("Text Schedule");
  const rows = sheet.getDataRange().getValues();

  const headers = rows[0];
  const data = rows.slice(1);

  const nameCol = headers.indexOf("Name");
  const manualStatusCol = 3; // Your first Status column, column D
  const daysSinceCol = headers.indexOf("Days Since Start");
  const daysTillEndCol = headers.indexOf("Days Till Task End");
  const currentCol = headers.indexOf("Current?");

  const currentRow = data.find(row => row[currentCol] === "Yes");

  if (!currentRow) {
    return ContentService
      .createTextOutput(JSON.stringify({
        error: "No current task found"
      }))
      .setMimeType(ContentService.MimeType.JSON);
  }

  const result = {
    name: currentRow[nameCol],
    status: currentRow[manualStatusCol],
    daysSinceStart: currentRow[daysSinceCol],
    daysTillTaskEnd: currentRow[daysTillEndCol]
  };

  return ContentService
    .createTextOutput(JSON.stringify(result))
    .setMimeType(ContentService.MimeType.JSON);
}

function formatDate(date) {
  if (!(date instanceof Date)) return date;

  return Utilities.formatDate(
    date,
    Session.getScriptTimeZone(),
    "M/d/yyyy"
  );
}
