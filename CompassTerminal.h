// ============================================================
// UART COMPASS COMMANDS
// ============================================================
// 500 = L       (LOAD)
// 501 = W       (WRITE)
// 502 = C       (CALIBRATION START)
// 503 = S       (STOP)
// 504 = CLEARCAL
// ============================================================


float pendingCompassCommand = 0.0f;



float compassCommandCode(const char *command)
{
  if (strcmp(command, "READ CAL.") == 0)        return 500.0f;
  if (strcmp(command, "WRITE CAL.") == 0)       return 501.0f;
  if (strcmp(command, "CALLIBRATE") == 0)  return 502.0f;
  if (strcmp(command, "STOP&SAFE") == 0)        return 503.0f;
  if (strcmp(command, "CLEARCAL") == 0)    return 504.0f;

  return 0.0f;
}

void sendCompassTerminalCommand(const char *command)
{
  float code = compassCommandCode(command);

  if (code == 0.0f)
    return;

  pendingCompassCommand = code;

  strncpy(
    compassTerminalLastCommand,
    command,
    sizeof(compassTerminalLastCommand) - 1);

  compassTerminalLastCommand[
    sizeof(compassTerminalLastCommand) - 1] = '\0';
}

void addCompassTerminalLine(const char *text)
{
  for (int i = 0; i < COMPASS_TERMINAL_LINES - 1; i++) {
    strncpy(
      compassTerminalLines[i],
      compassTerminalLines[i + 1],
      COMPASS_TERMINAL_LINE_LENGTH - 1);

    compassTerminalLines[i][
      COMPASS_TERMINAL_LINE_LENGTH - 1] = '\0';
  }

  strncpy(
    compassTerminalLines[COMPASS_TERMINAL_LINES - 1],
    text,
    COMPASS_TERMINAL_LINE_LENGTH - 1);

  compassTerminalLines[COMPASS_TERMINAL_LINES - 1][
    COMPASS_TERMINAL_LINE_LENGTH - 1] = '\0';

  strncpy(
    compassTerminalResponse,
    text,
    COMPASS_TERMINAL_LINE_LENGTH - 1);

  compassTerminalResponse[
    COMPASS_TERMINAL_LINE_LENGTH - 1] = '\0';
}
