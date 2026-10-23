/* initech-0xs6, audit DAS L022: a refused quit must explain the cancellation
 * on screen and leave the application resident. IM VI PDF p.345 supplies
 * cancellable shutdown; wording is authored, not a claimed native string.
 * Three lines fit the existing caution-alert text column; app is DOS 8.3. */
#ifndef INITECH_FLAIR_POWER_NOTICE_H
#define INITECH_FLAIR_POWER_NOTICE_H
static inline unsigned finder_power_notice(int asked, const char *app,
                                           const char *lines[3])
{
#ifdef POWER_MUT_SILENT_CANCEL
    (void)asked; (void)app; (void)lines;
    return 0;
#else
    lines[0] = "Shut Down or Restart was cancelled.";
    lines[1] = app;
    lines[2] = asked ? "The application did not quit." :
                       "Close this application and try again.";
    return 3;
#endif
}
#endif
