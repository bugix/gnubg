/*
 * Copyright (C) 2002-2004 Joern Thyssen <jthyssen@dk.ibm.com>
 * Copyright (C) 2002-2019 the AUTHORS
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <stdarg.h>

#include "backgammon.h"
#include "drawboard.h"
#include "export.h"
#include "format.h"
#include "positionid.h"
#include "matchid.h"
#include "relational.h"


/* "Color" of chequers */

static void
printTextBoard(FILE * pf, const matchstate * pms)
{

    TanBoard anBoard;
    char szBoard[2048];
    char sz[32], szCube[40], szPlayer0[MAX_NAME_LEN + 3], szPlayer1[MAX_NAME_LEN + 3],
        szScore0[35], szScore1[35], szMatch[35];
    char *apch[7] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL };
    unsigned int anPips[2];

    memcpy(anBoard, pms->anBoard, sizeof(anBoard));

    apch[0] = szPlayer0;
    apch[6] = szPlayer1;

    sprintf(apch[1] = szScore0, ngettext("%d point", "%d points", pms->anScore[0]), pms->anScore[0]);

    sprintf(apch[5] = szScore1, ngettext("%d point", "%d points", pms->anScore[1]), pms->anScore[1]);

    if (pms->fDoubled) {
        apch[pms->fTurn ? 4 : 2] = szCube;

        sprintf(szPlayer0, "O: %s", ap[0].szName);
        sprintf(szPlayer1, "X: %s", ap[1].szName);
        sprintf(szCube, _("Cube offered at %d"), pms->nCube << 1);
    } else {
        sprintf(szPlayer0, "O: %s", ap[0].szName);
        sprintf(szPlayer1, "X: %s", ap[1].szName);

        apch[pms->fMove ? 4 : 2] = sz;

        if (pms->anDice[0])
            sprintf(sz, _("Rolled %u%u"), pms->anDice[0], pms->anDice[1]);
        else if (!GameStatus((ConstTanBoard) anBoard, pms->bgv))
            strcpy(sz, _("On roll"));
        else
            sz[0] = 0;

        if (pms->fCubeOwner < 0) {
            apch[3] = szCube;

            if (pms->nMatchTo)
                if (pms->nMatchTo == 1)
                    sprintf(szCube, ngettext("%d point match", "%d points match", pms->nMatchTo), pms->nMatchTo);
                else if (pms->fCrawford) {
                    sprintf(szCube, ngettext("%d point match", "%d points match", pms->nMatchTo), pms->nMatchTo);
                    strcat(szCube, " (");
                    strcat(szCube, _("Crawford game"));
                    strcat(szCube, ")");
                 } else
                    sprintf(szCube, ngettext("%d point match (Cube: %d)", "%d points match (Cube: %d)", pms->nMatchTo),
                            pms->nMatchTo, pms->nCube);
            else
                sprintf(szCube, _("(Cube: %d)"), pms->nCube);
        } else {
            sprintf(szCube, _("%c: %.20s (Cube: %d)"), pms->fCubeOwner ? 'X' :
                    'O', ap[pms->fCubeOwner].szName, pms->nCube);

            apch[pms->fCubeOwner ? 6 : 0] = szCube;

            if (pms->nMatchTo)
                sprintf(apch[3] = szMatch, ngettext("%d point match", "%d points match", pms->nMatchTo), pms->nMatchTo);
        }
    }



    if (pms->fResigned)
        sprintf(strchr(sz, 0), _(", resigns %s"), gettext(aszGameResult[pms->fResigned - 1]));

    if (!pms->fMove)
        SwapSides(anBoard);


    fputs(DrawBoard(szBoard, (ConstTanBoard) anBoard, pms->fMove, apch,
                    MatchIDFromMatchState(pms), anChequers[ms.bgv]), pf);

    PipCount((ConstTanBoard) anBoard, anPips);

    fprintf(pf, "%s O %u, X %u\n\n", _("Pip counts:"), anPips[0], anPips[1]);

}


/*
 * Print text header for board: move or cube decision 
 *
 * Input:
 *   pf: output file
 *   ms: current match state
 *   iMove: move no.
 *
 */

extern void
TextBoardHeader(GString * gsz, const matchstate * pms, const int UNUSED(iGame), const int iMove)
{

    if (iMove >= 0)
        g_string_append_printf(gsz, _("Move number %d: "), iMove + 1);

    if (pms->fResigned)
        /* resignation */

        g_string_append_printf(gsz,
                               ngettext(" %s resigns %d point",
                                        " %s resigns %d points",
                                        pms->fResigned * pms->nCube),
                               ap[pms->fTurn].szName, pms->fResigned * pms->nCube);

    else if (pms->anDice[0] && pms->anDice[1])
        /* chequer play decision */

        g_string_append_printf(gsz, _(" %s to play %u%u\n\n"), ap[pms->fMove].szName, pms->anDice[0], pms->anDice[1]
            );

    else if (pms->fDoubled)
        /* take decision */

        g_string_append_printf(gsz, _(" %s doubles to %d\n\n"), ap[!(pms->fTurn)].szName, pms->nCube * 2);

    else
        /* cube decision */

        g_string_append_printf(gsz, _(" %s on roll, cube decision?\n\n"), ap[pms->fMove].szName);

}


/*
 * Print text header
 *
 * Input:
 *   pf: output file
 *   ms: current match state
 *
 */

extern void
TextPrologue(GString * gsz, const matchstate * pms, const int UNUSED(iGame))
{

    g_string_append_printf(gsz,
                           ngettext("The score (after %d game) is: %s %d, %s %d",
                                    "The score (after %d games) is: %s %d, %s %d", pms->cGames), pms->cGames,
                           ap[0].szName, pms->anScore[0], ap[1].szName, pms->anScore[1]);

    if (pms->nMatchTo > 0) {
        g_string_append_printf(gsz,
                               ngettext(" (match to %d point)", " (match to %d points)", pms->nMatchTo), pms->nMatchTo);
        if (pms->nMatchTo > 1) {
            if (pms->fCrawford)
                g_string_append(gsz, _(", Crawford game"));
            if (pms->fPostCrawford)
                g_string_append(gsz, _(", post-Crawford play"));
        }
    }

    g_string_append(gsz, "\n\n");

}


/*
 * Print text footer
 *
 * Input:
 *   pf: output file
 *   ms: current match state
 *
 */

static void
TextEpilogue(FILE * pf, const matchstate * UNUSED(pms))
{

    time_t t;
    char tstr[11];              /* ISO 8601 date format: YYYY-MM-DD\0 */

    time(&t);
    strftime(tstr, 11, "%Y-%m-%d", localtime(&t));

    fprintf(pf, _("Output generated %s by %s\n\n"), tstr, VERSION_STRING);

}

/*
 * Print cube analysis
 *
 * Input:
 *  pf: output file
 *  arDouble: equitites for cube decisions
 *  fPlayer: player who doubled
 *  esDouble: eval setup
 *  pci: cubeinfo
 *  fDouble: double/no double
 *  fTake: take/drop
 *
 */

static void
TextPrintCubeAnalysisTable(GString * gsz,
                           float aarOutput[2][NUM_ROLLOUT_OUTPUTS],
                           float aarStdDev[2][NUM_ROLLOUT_OUTPUTS],
                           int UNUSED(fPlayer),
                           const evalsetup * pes, const cubeinfo * pci,
                           int fDouble, int fTake, skilltype stDouble, skilltype stTake)
{

    int fActual, fClose, fMissed;
    int fDisplayIt;
    float arDouble[4];

    /* check if cube analysis should be printed */

    if (pes->et == EVAL_NONE)
        return;                 /* no evaluation */

    FindCubeDecision(arDouble, aarOutput, pci);

    fActual = fDouble;
    fClose = isCloseCubedecision(arDouble);
    fMissed = isMissedDouble(arDouble, aarOutput, fDouble, pci);

    fDisplayIt =
        (fActual && exsExport.afCubeDisplay[EXPORT_CUBE_ACTUAL]) ||
        (fClose && exsExport.afCubeDisplay[EXPORT_CUBE_CLOSE]) ||
        (fMissed && exsExport.afCubeDisplay[EXPORT_CUBE_MISSED]) ||
        (exsExport.afCubeDisplay[stDouble]) || (exsExport.afCubeDisplay[stTake]);

    if (!fDisplayIt)
        return;

    g_string_append(gsz, OutputCubeAnalysisFull(aarOutput, aarStdDev, pes, pci, fDouble, fTake, stDouble, stTake));

}



/*
 * Wrapper for print cube analysis
 *
 * Input:
 *  pf: output file
 *  pms: match state
 *  pmr: current move record
 *  szImageDir: URI to images
 *  szExtension: extension of images
 *  fTake: take/drop
 *
 */

static void
TextPrintCubeAnalysis(GString * gsz, const matchstate * pms, moverecord * pmr)
{

    cubeinfo ci;
    /* we need to remember the double type to be able to do the right
     * thing for beavers and racoons */
    static doubletype dt = DT_NORMAL;

    GetMatchStateCubeInfo(&ci, pms);


    switch (pmr->mt) {

    case MOVE_NORMAL:

        /* cube analysis from move */

        TextPrintCubeAnalysisTable(gsz,
                                   pmr->CubeDecPtr->aarOutput,
                                   pmr->CubeDecPtr->aarStdDev,
                                   pmr->fPlayer, &pmr->CubeDecPtr->esDouble, &ci, FALSE, -1, pmr->stCube, SKILL_NONE);
        dt = DT_NORMAL;

        break;

    case MOVE_DOUBLE:

        dt = DoubleType(pms->fDoubled, pms->fMove, pms->fTurn);
        if (dt != DT_NORMAL) {
            g_string_append(gsz, _("Cannot analyse beaver nor raccoons!"));
            g_string_append(gsz, "\n");
            break;
        }
        TextPrintCubeAnalysisTable(gsz,
                                   pmr->CubeDecPtr->aarOutput,
                                   pmr->CubeDecPtr->aarStdDev,
                                   pmr->fPlayer, &pmr->CubeDecPtr->esDouble, &ci, TRUE, -1, pmr->stCube, SKILL_NONE);

        break;

    case MOVE_TAKE:
    case MOVE_DROP:

        /* cube analysis from double, {take, drop, beaver} */

        if (dt != DT_NORMAL) {
            dt = DT_NORMAL;
            g_string_append(gsz, _("Cannot analyse beaver nor raccoons!"));
            g_string_append(gsz, "\n");
            break;
        }
        TextPrintCubeAnalysisTable(gsz, pmr->CubeDecPtr->aarOutput, pmr->CubeDecPtr->aarStdDev, pmr->fPlayer, &pmr->CubeDecPtr->esDouble, &ci, TRUE, pmr->mt == MOVE_TAKE, SKILL_NONE,  /* FIXME: skill from prev. cube */
                                   pmr->stCube);

        break;

    default:

        g_assert_not_reached();


    }

    return;

}


/*
 * Print move analysis
 *
 * Input:
 *  pf: output file
 *  pms: match state
 *  pmr: current move record
 *  szImageDir: URI to images
 *  szExtension: extension of images
 *
 */

static void
TextPrintMoveAnalysis(GString * gsz, const matchstate * pms, moverecord * pmr)
{
    char sz[FORMATEDMOVESIZE];

    cubeinfo ci;

    GetMatchStateCubeInfo(&ci, pms);

    /* check if move should be printed */

    if (!exsExport.afMovesDisplay[pmr->n.stMove])
        return;

    /* print alerts */

    if (badSkill(pmr->n.stMove)) {

        /* blunder or error */

        g_string_append_printf(gsz, _("Alert: %s move"), gettext(aszSkillType[pmr->n.stMove]));

        if (!pms->nMatchTo || !fOutputMWC)
            g_string_append_printf(gsz, " (%+*.*f)\n",
                                   fOutputDigits + 4, fOutputDigits,
                                   pmr->ml.amMoves[pmr->n.iMove].rScore - pmr->ml.amMoves[0].rScore);
        else
            g_string_append_printf(gsz, " (%+*.*f%%)\n",
                                   fOutputDigits + 3, fOutputDigits,
                                   100.0f *
                                   eq2mwc(pmr->ml.amMoves[pmr->n.iMove].rScore, &ci) -
                                   100.0f * eq2mwc(pmr->ml.amMoves[0].rScore, &ci));

    }

    if (pmr->lt != LUCK_NONE) {

        /* joker */

        g_string_append_printf(gsz, _("Alert: %s roll!"), gettext(aszLuckType[pmr->lt]));

        if (!pms->nMatchTo || !fOutputMWC)
            g_string_append_printf(gsz, " (%+*.*f)\n", fOutputDigits + 4, fOutputDigits, pmr->rLuck);
        else
            g_string_append_printf(gsz, " (%+*.*f%%)\n",
                                   fOutputDigits + 3, fOutputDigits,
                                   100.0f * eq2mwc(pmr->rLuck, &ci) - 100.0f * eq2mwc(0.0f, &ci));

    }

    g_string_append(gsz, "\n");

    g_string_append_printf(gsz, _("Rolled %u%u"), pmr->anDice[0], pmr->anDice[1]);

    if (pmr->rLuck != ERR_VAL)
        g_string_append_printf(gsz, " (%s):\n", GetLuckAnalysis(pms, pmr->rLuck));
    else
        g_string_append_printf(gsz, ":\n");

    if (pmr->ml.cMoves) {
        unsigned int i;

        for (i = 0; i < pmr->ml.cMoves; i++) {
            char szBuf[1024];

            if (i >= exsExport.nMoves && i != pmr->n.iMove)
                continue;

            g_string_append(gsz, i == pmr->n.iMove ? "*" : " ");
            g_string_append(gsz, FormatMoveHint(szBuf, pms, &pmr->ml, i,
                                                i != pmr->n.iMove ||
                                                i != pmr->ml.cMoves - 1 ||
                                                pmr->ml.cMoves == 1 ||
                                                i < exsExport.nMoves,
                                                exsExport.fMovesDetailProb,
                                                exsExport.afMovesParameters[pmr->ml.amMoves[i].esMove.et - 1]));


        }

    } else {

        if (pmr->n.anMove[0] >= 0)
            /* no movelist saved */
            g_string_append_printf(gsz, "*    %s\n", FormatMove(sz, pms->anBoard, pmr->n.anMove));
        else
            /* no legal moves */
            /* FIXME: output equity?? */
            g_string_append_printf(gsz, "*    %s\n", _("Cannot move"));

    }

    g_string_append(gsz, "\n\n");

    return;

}





/*
 * Print cube analysis
 *
 * Input:
 *  pf: output file
 *  pms: match state
 *  pmr: current move record
 *
 */

extern void
TextAnalysis(GString * gsz, const matchstate * pms, moverecord * pmr)
{

    switch (pmr->mt) {

    case MOVE_NORMAL:

        if (pmr->n.anMove[0] >= 0) {
            char sz[FORMATEDMOVESIZE];

            g_string_append_printf(gsz,
                                   _("* %s moves %s"),
                                   ap[pmr->fPlayer].szName, FormatMove(sz, pms->anBoard, pmr->n.anMove));
        } else if (!pmr->ml.cMoves)
            g_string_append_printf(gsz, _("* %s cannot move"), ap[pmr->fPlayer].szName);

        g_string_append(gsz, "\n");

        if (exsExport.fIncludeAnalysis) {
            TextPrintCubeAnalysis(gsz, pms, pmr);

            TextPrintMoveAnalysis(gsz, pms, pmr);
        }

        break;

    case MOVE_DOUBLE:
    case MOVE_TAKE:
    case MOVE_DROP:

        if (pmr->mt == MOVE_DOUBLE)
            g_string_append_printf(gsz, "* %s %s\n\n", ap[pmr->fPlayer].szName, _("doubles"));
        else
            g_string_append_printf(gsz,
                                   "* %s %s\n\n",
                                   ap[pmr->fPlayer].szName, (pmr->mt == MOVE_TAKE) ? _("accepts") : _("rejects"));

        if (exsExport.fIncludeAnalysis)
            TextPrintCubeAnalysis(gsz, pms, pmr);

        break;

    default:
        break;

    }

}


static void
TextDumpStatcontext(GString * gsz, const statcontext * psc, int nMatchTo)
{
    char sz[STATCONTEXT_MAXSIZE];

    DumpStatcontext(sz, psc, ap[0].szName, ap[1].szName, nMatchTo);
    g_string_append(gsz, sz);
    g_string_append(gsz, "\n\n");

}



static void
TextPrintComment(FILE * pf, const moverecord * pmr)
{

    char *sz = pmr->sz;


    if (sz) {

        fputs(_("Annotation:\n"), pf);
        fputs(sz, pf);
        fputs("\n", pf);

    }


}

static void
TextMatchInfo(FILE * pf, const matchinfo * pmi)
{

    int i;

    fputs(_("Match Information:\n"), pf);

    /* ratings */

    for (i = 0; i < 2; ++i) {
        if (pmi->pchRating[i]) {
            fprintf(pf, _("%s's rating: %s\n"), ap[i].szName, pmi->pchRating[i]);
        }
    }

    /* date */

    if (pmi->nYear) {
        /* the fields below are not used here but are read
         * inside strftime(), so initialise them.
         */
        struct tm tmx = { .tm_sec=0, .tm_min=0, .tm_hour=0, .tm_isdst=(-1) };
        char sz[80];

        tmx.tm_year = pmi->nYear - 1900;
        tmx.tm_mon = pmi->nMonth - 1;
        tmx.tm_mday = pmi->nDay;
        strftime(sz, sizeof(sz), "%x", &tmx);
        fprintf(pf, _("Date: %s\n"), sz);

    }
    /* else fputs ( _("Date: n/a\n"), pf ); */

    /* event, round, place and annotator */

    if (pmi->pchEvent) {
        fprintf(pf, _("Event: %s\n"), pmi->pchEvent);
    }

    if (pmi->pchRound) {
        fprintf(pf, _("Round: %s\n"), pmi->pchRound);
    }

    if (pmi->pchPlace) {
        fprintf(pf, _("Place: %s\n"), pmi->pchPlace);
    }

    if (pmi->pchAnnotator) {
        fprintf(pf, _("Annotator: %s\n"), pmi->pchAnnotator);
    }
    if (pmi->pchComment) {
        fprintf(pf, _("Comments: %s\n"), pmi->pchComment);
    }
}

/*
 * Export a game in text
 *
 * Input:
 *   pf: output file
 *   plGame: list of moverecords for the current game
 *
 */

static void
ExportGameText(FILE * pf, listOLD * plGame, const int iGame, const int fLastGame)
{

    listOLD *pl;
    moverecord *pmr;
    matchstate msExport;
    matchstate msOrig;
    int iMove = 0;
    statcontext *psc = NULL;
    static statcontext scTotal;
    xmovegameinfo *pmgi = NULL;
    GString *gsz;
    listOLD *pl_hint = NULL;

    msOrig.nMatchTo = 0;

    if (!iGame)
        IniStatcontext(&scTotal);

    updateStatisticsGame(plGame);

    if (game_is_last(plGame))
        pl_hint = game_add_pmr_hint(plGame);

    for (pl = plGame->plNext; pl != plGame; pl = pl->plNext) {

        pmr = pl->p;

        FixMatchState(&msExport, pmr);

        switch (pmr->mt) {

        case MOVE_GAMEINFO:

            ApplyMoveRecord(&msExport, plGame, pmr);

            gsz = g_string_new(NULL);
            TextPrologue(gsz, &msExport, iGame);
            fputs(gsz->str, pf);
            g_string_free(gsz, TRUE);

            if (exsExport.fIncludeMatchInfo) {
                TextMatchInfo(pf, &mi);
                fputs("\n", pf);
            }

            msOrig = msExport;
            pmgi = &pmr->g;

            psc = &pmr->g.sc;

            AddStatcontext(psc, &scTotal);

            /* FIXME: game introduction */
            break;

        case MOVE_NORMAL:

            if (pmr->fPlayer != msExport.fMove)
                SwapSides(msExport.anBoard);

            msExport.fTurn = msExport.fMove = pmr->fPlayer;
            msExport.anDice[0] = pmr->anDice[0];
            msExport.anDice[1] = pmr->anDice[1];

            gsz = g_string_new(NULL);
            TextBoardHeader(gsz, &msExport, iGame, iMove);
            fputs(gsz->str, pf);
            g_string_free(gsz, TRUE);

            printTextBoard(pf, &msExport);
            gsz = g_string_new(NULL);
            TextAnalysis(gsz, &msExport, pmr);
            fputs(gsz->str, pf);
            g_string_free(gsz, TRUE);

            iMove++;

            break;

        case MOVE_DOUBLE:
        case MOVE_TAKE:
        case MOVE_DROP:

            gsz = g_string_new(NULL);
            TextBoardHeader(gsz, &msExport, iGame, iMove);
            fputs(gsz->str, pf);
            g_string_free(gsz, TRUE);

            printTextBoard(pf, &msExport);

            gsz = g_string_new(NULL);
            TextAnalysis(gsz, &msExport, pmr);
            fputs(gsz->str, pf);
            g_string_free(gsz, TRUE);

            iMove++;

            break;

        default:

            break;

        }

        if (exsExport.fIncludeAnnotation)
            TextPrintComment(pf, pmr);

        ApplyMoveRecord(&msExport, plGame, pmr);

    }

    if (pl_hint)
        game_remove_pmr_hint(pl_hint);

    if (pmgi && pmgi->fWinner != -1) {

        /* print game result */

        fprintf(pf,
                ngettext("%s wins %d point", "%s wins %d points",
                         pmgi->nPoints), ap[pmgi->fWinner].szName, pmgi->nPoints);
    }

    if (psc) {
        gsz = g_string_new(NULL);
        g_string_append_printf(gsz, _("\n\nGame statistics for game %d\n\n"), iGame + 1);
        TextDumpStatcontext(gsz, psc, msOrig.nMatchTo);
        fputs(gsz->str, pf);
        g_string_free(gsz, TRUE);
    }

    if (fLastGame) {
        statcontext *psc_rel = relational_player_stats_get(ap[0].szName, ap[1].szName);

        gsz = g_string_new(NULL);
        if (msOrig.nMatchTo)
            g_string_append_printf(gsz, _("Match statistics\n\n"));
        else
            g_string_append_printf(gsz, _("Session statistics\n\n"));
        TextDumpStatcontext(gsz, &scTotal, msOrig.nMatchTo);

        if (psc_rel) {
            g_string_append_printf(gsz, _("\nStatistics from database\n\n"));
            TextDumpStatcontext(gsz, psc_rel, 0);
            g_free(psc_rel);
        }
        fputs(gsz->str, pf);
        g_string_free(gsz, TRUE);
    }

    TextEpilogue(pf, &msExport);
}

extern void
CommandExportGameText(char *sz)
{

    FILE *pf;
    int fDontClose = FALSE;

    sz = NextToken(&sz);

    if (!plGame) {
        outputl(_("No game in progress (type `new game' to start one)."));
        return;
    }

    if (!sz || !*sz) {
        outputl(_("You must specify a file to export to (see `help export game text')."));
        return;
    }

    if (!confirmOverwrite(sz, fConfirmSave))
        return;

    if (!strcmp(sz, "-")) {
        pf = stdout;
        fDontClose = TRUE;
    } else if ((pf = g_fopen(sz, "w")) == 0) {
        outputerr(sz);
        return;
    }

    ExportGameText(pf, plGame, getGameNumber(plGame), FALSE);

    if (!fDontClose)
        fclose(pf);

    setDefaultFileName(sz);
}

extern void
CommandExportMatchText(char *sz)
{

    FILE *pf;
    listOLD *pl;
    int nGames;
    int i;
    int fDontClose = FALSE;

    sz = NextToken(&sz);

    if (!sz || !*sz) {
        outputl(_("You must specify a file to export to (see `help export match text')."));
        return;
    }

    /* Find number of games in match */
    for (pl = lMatch.plNext, nGames = 0; pl != &lMatch; pl = pl->plNext, nGames++);

    for (pl = lMatch.plNext, i = 0; pl != &lMatch; pl = pl->plNext, i++) {

        char *szCurrent = filename_from_iGame(sz, i);

        if (!i) {

            if (!confirmOverwrite(sz, fConfirmSave)) {
                g_free(szCurrent);
                return;
            }

            setDefaultFileName(sz);

        }


        if (!strcmp(szCurrent, "-")) {
            pf = stdout;
            fDontClose = TRUE;
        } else if ((pf = g_fopen(szCurrent, "w")) == 0) {
            outputerr(szCurrent);
            g_free(szCurrent);
            return;
        }

        ExportGameText(pf, pl->p, i, i == nGames - 1);

        if (!fDontClose)
            fclose(pf);

        g_free(szCurrent);
    }

}



extern void
CommandExportPositionText(char *sz)
{

    FILE *pf;
    int fHistory;
    moverecord *pmr;
    int iMove;
    GString *gsz;
    int fDontClose = FALSE;

    sz = NextToken(&sz);

    if (ms.gs == GAME_NONE) {
        outputl(_("No game in progress (type `new game' to start one)."));
        return;
    }

    if (!sz || !*sz) {
        outputl(_("You must specify a file to export to (see `help export position text')."));
        return;
    }
    pmr = get_current_moverecord(&fHistory);

    if (!confirmOverwrite(sz, fConfirmSave))
        return;

    if (!strcmp(sz, "-")) {
        pf = stdout;
        fDontClose = TRUE;
    } else if ((pf = g_fopen(sz, "w")) == 0) {
        outputerr(sz);
        return;
    }

    gsz = g_string_new(NULL);
    TextPrologue(gsz, &ms, getGameNumber(plGame));
    fputs(gsz->str, pf);
    g_string_free(gsz, TRUE);

    if (exsExport.fIncludeMatchInfo)
        TextMatchInfo(pf, &mi);

    if (fHistory)
        iMove = getMoveNumber(plGame, pmr) - 1;
    else if (plLastMove)
        iMove = getMoveNumber(plGame, plLastMove->p);
    else
        iMove = -1;

    gsz = g_string_new(NULL);
    TextBoardHeader(gsz, &ms, getGameNumber(plGame), iMove);
    fputs(gsz->str, pf);
    g_string_free(gsz, TRUE);


    printTextBoard(pf, &ms);

    if (pmr) {

        gsz = g_string_new(NULL);
        TextAnalysis(gsz, &ms, pmr);
        fputs(gsz->str, pf);
        g_string_free(gsz, TRUE);

        if (exsExport.fIncludeAnnotation)
            TextPrintComment(pf, pmr);

    }

    TextEpilogue(pf, &ms);

    if (!fDontClose)
        fclose(pf);

    setDefaultFileName(sz);

}
