#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "editorlist.h"
#include "editor.h"
#include "systemconsts.h"
#include "settings.h"
#include "qsynedit/Constants.h"

#include <QCloseEvent>
#include <QComboBox>
#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>
#include <QTranslator>

#include "settingsdialog/settingsdialog.h"
#include "compiler/compilermanager.h"
#include <QGuiApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QTextCodec>
#include <QDebug>

MainWindow* pMainWindow;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      mMessageControlChanged(false),
      mTabMessagesTogglingState(false),
      mCheckSyntaxInBack(false)
{
    ui->setupUi(this);
    // status bar
    mFileInfoStatus=new QLabel();
    mFileEncodingStatus = new QLabel();
    mFileModeStatus = new QLabel();
    mFileInfoStatus->setStyleSheet("margin-left:10px; margin-right:10px");
    mFileEncodingStatus->setStyleSheet("margin-left:10px; margin-right:10px");
    mFileModeStatus->setStyleSheet("margin-left:10px; margin-right:10px");
    ui->statusbar->addWidget(mFileInfoStatus);
    ui->statusbar->addWidget(mFileEncodingStatus);
    ui->statusbar->addWidget(mFileModeStatus);
    mEditorList = new EditorList(ui->EditorTabsLeft,
                                 ui->EditorTabsRight,
                                 ui->splitterEditorPanel,
                                 ui->EditorPanel);
    setupActions();
    ui->EditorTabsRight->setVisible(false);

    mCompilerSet = new QComboBox();
    mCompilerSet->setMinimumWidth(200);
    ui->toolbarCompilerSet->addWidget(mCompilerSet);
    connect(mCompilerSet,QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onCompilerSetChanged);
    updateCompilerSet();

    mCompilerManager = new CompilerManager(this);

    ui->actionIndent->setShortcut(Qt::Key_Tab);
    ui->actionUnIndent->setShortcut(Qt::Key_Tab | Qt::ShiftModifier);

    ui->tableIssues->setErrorColor(QColor("Red"));
    ui->tableIssues->setWarningColor(QColor("Orange"));

    mMenuEncoding = new QMenu();
    mMenuEncoding->setTitle(tr("File Encoding"));
    mMenuEncoding->addAction(ui->actionAuto_Detect);
    mMenuEncoding->addAction(ui->actionEncode_in_ANSI);
    mMenuEncoding->addAction(ui->actionEncode_in_UTF_8);
    mMenuEncoding->addSeparator();
    mMenuEncoding->addAction(ui->actionConvert_to_ANSI);
    mMenuEncoding->addAction(ui->actionConvert_to_UTF_8);
    ui->menuEdit->insertMenu(ui->actionFoldAll,mMenuEncoding);
    ui->menuEdit->insertSeparator(ui->actionFoldAll);
    ui->actionAuto_Detect->setCheckable(true);
    ui->actionEncode_in_ANSI->setCheckable(true);
    ui->actionEncode_in_UTF_8->setCheckable(true);

    updateEditorActions();
    applySettings();

    openCloseMessageSheet(false);
    mPreviousHeight = 250;

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateForEncodingInfo() {
    Editor * editor = mEditorList->getEditor();
    if (editor!=NULL) {
        mFileEncodingStatus->setText(
                    QString("%1(%2)")
                    .arg(QString(editor->encodingOption()))
                    .arg(QString(editor->fileEncoding())));
        ui->actionAuto_Detect->setChecked(editor->encodingOption() == ENCODING_AUTO_DETECT);
        ui->actionEncode_in_ANSI->setChecked(editor->encodingOption() == ENCODING_SYSTEM_DEFAULT);
        ui->actionEncode_in_UTF_8->setChecked(editor->encodingOption() == ENCODING_UTF8);
    } else {
        mFileEncodingStatus->setText("");
        ui->actionAuto_Detect->setChecked(false);
        ui->actionEncode_in_ANSI->setChecked(false);
        ui->actionEncode_in_UTF_8->setChecked(false);
    }
}

void MainWindow::updateEditorSettings()
{
    mEditorList->applySettings();
}

void MainWindow::updateEditorActions()
{
    Editor* e = mEditorList->getEditor();
    if (e==nullptr) {
        ui->actionAuto_Detect->setEnabled(false);
        ui->actionEncode_in_ANSI->setEnabled(false);
        ui->actionEncode_in_UTF_8->setEnabled(false);
        ui->actionConvert_to_ANSI->setEnabled(false);
        ui->actionConvert_to_UTF_8->setEnabled(false);
        ui->actionCompile->setEnabled(false);
        ui->actionCopy->setEnabled(false);
        ui->actionCut->setEnabled(false);
        ui->actionFoldAll->setEnabled(false);
        ui->actionIndent->setEnabled(false);
        ui->actionPaste->setEnabled(false);
        ui->actionRedo->setEnabled(false);
        ui->actionRun->setEnabled(false);
        ui->actionSave->setEnabled(false);
        ui->actionSaveAs->setEnabled(false);
        ui->actionSaveAll->setEnabled(false);
        ui->actionSelectAll->setEnabled(false);
        ui->actionToggleComment->setEnabled(false);
        ui->actionUnIndent->setEnabled(false);
        ui->actionUndo->setEnabled(false);
        ui->actionUnfoldAll->setEnabled(false);
    } else {
        ui->actionAuto_Detect->setEnabled(true);
        ui->actionEncode_in_ANSI->setEnabled(true);
        ui->actionEncode_in_UTF_8->setEnabled(true);
        ui->actionConvert_to_ANSI->setEnabled(e->encodingOption()!=ENCODING_SYSTEM_DEFAULT && e->fileEncoding()!=ENCODING_SYSTEM_DEFAULT);
        ui->actionConvert_to_UTF_8->setEnabled(e->encodingOption()!=ENCODING_UTF8 && e->fileEncoding()!=ENCODING_UTF8);
        //if (e->compilable())
            ui->actionCompile->setEnabled(true);
        ui->actionRun->setEnabled(true);
        ui->actionCopy->setEnabled(e->selAvail());
        ui->actionCut->setEnabled(e->selAvail());
        ui->actionFoldAll->setEnabled(e->lines()->count()>0);
        ui->actionIndent->setEnabled(!e->readOnly());

        ui->actionPaste->setEnabled(!e->readOnly() && !QGuiApplication::clipboard()->text().isEmpty());
        ui->actionRedo->setEnabled(e->canRedo());
        ui->actionUndo->setEnabled(e->canUndo());
        ui->actionSave->setEnabled(e->modified());
        ui->actionSaveAs->setEnabled(true);
        ui->actionSaveAll->setEnabled(true);
        ui->actionSelectAll->setEnabled(e->lines()->count()>0);
        ui->actionToggleComment->setEnabled(!e->readOnly() && e->lines()->count()>0);
        ui->actionUnIndent->setEnabled(!e->readOnly() && e->lines()->count()>0);
        ui->actionUnfoldAll->setEnabled(e->lines()->count()>0);
    }

}

void MainWindow::updateEditorColorSchemes()
{
    mEditorList->applyColorSchemes(pSettings->editor().colorScheme());
}

void MainWindow::applySettings()
{
    changeTheme(pSettings->environment().theme());
    QFont font(pSettings->environment().interfaceFont(),
               pSettings->environment().interfaceFontSize());
    font.setStyleStrategy(QFont::PreferAntialias);
    QApplication * app = dynamic_cast<QApplication*>(QApplication::instance());
    app->setFont(font);
    this->setFont(font);
}

void MainWindow::updateStatusbarForLineCol()
{
    Editor* e = mEditorList->getEditor();
    if (e!=nullptr) {
        QString msg = tr("Line:%1    Col:%2    Selected:%3    Lines:%4    Length:%5")
                .arg(e->caretY(),6)
                .arg(e->caretX(),6)
                .arg(e->selText().length(),6)
                .arg(e->lines()->count(),6)
                .arg(e->lines()->getTextLength(),6);
        mFileInfoStatus->setText(msg);
    } else {
        mFileInfoStatus->setText("");
    }
}

void MainWindow::updateForStatusbarModeInfo()
{
    Editor* e = mEditorList->getEditor();
    if (e!=nullptr) {
        QString msg;
        if (e->readOnly()) {
            msg = tr("Read Only");
        } else if (e->insertMode()) {
            msg = tr("Insert");
        } else {
            msg = tr("Overwrite");
        }
        mFileModeStatus->setText(msg);
    } else {
        mFileModeStatus->setText("");
    }
}

void MainWindow::openFiles(const QStringList &files)
{
    mEditorList->beginUpdate();
    auto end = finally([this] {
        this->mEditorList->endUpdate();
    });
    for (QString file:files) {
        openFile(file);
    }
    mEditorList->endUpdate();
}

void MainWindow::openFile(const QString &filename)
{
    Editor* editor = mEditorList->getOpenedEditorByFilename(filename);
    if (editor!=nullptr) {
        editor->activate();
        return;
    }
    editor = mEditorList->newEditor(filename,ENCODING_AUTO_DETECT,
                                    false,false);
    editor->activate();
    this->updateForEncodingInfo();
}

void MainWindow::setupActions() {

}

void MainWindow::updateCompilerSet()
{
    mCompilerSet->clear();
    int index=pSettings->compilerSets().defaultIndex();
    for (size_t i=0;i<pSettings->compilerSets().list().size();i++) {
        mCompilerSet->addItem(pSettings->compilerSets().list()[i]->name());
    }
    if (index < 0 || index>=mCompilerSet->count()) {
        index = 0;
    }
    mCompilerSet->setCurrentIndex(index);
}

void MainWindow::checkSyntaxInBack(Editor *e)
{
    if (e==nullptr)
        return;

//    if not devEditor.AutoCheckSyntax then
//      Exit;
    //not c or cpp file
    if (!e->highlighter() || e->highlighter()->getName()!=SYN_HIGHLIGHTER_CPP)
        return;
    if (mCompilerManager->backgroundSyntaxChecking())
        return;
    if (mCompilerManager->compiling())
        return;
//    if not Assigned(devCompilerSets.CompilationSet) then
//      Exit;
//    if fCompiler.Compiling then
//      Exit;
//    if fSyntaxChecker.Compiling then
//      Exit;
    if (mCheckSyntaxInBack)
        return;

    mCheckSyntaxInBack=true;
    e->clearSyntaxIssues();
    ui->tableIssues->clearIssues();
    mCompilerManager->checkSyntax(e->filename(),e->lines()->text());
//    if not PrepareForCompile(cttStdin,True) then begin
//      fCheckSyntaxInBack:=False;
//      Exit;
//    end;
//    if e.InProject then begin
//      if not assigned(MainForm.fProject) then
//        Exit;
//      fSyntaxChecker.Project := MainForm.fProject;
//    end;
    //    fSyntaxChecker.CheckSyntax(True);
}

bool MainWindow::compile(bool rebuild)
{
    Editor * editor = mEditorList->getEditor();
    if (editor != NULL ) {
        editor->clearSyntaxIssues();
        ui->tableIssues->clearIssues();
        if (editor->modified()) {
            if (!editor->save(false,false))
                return false;
        }
        if (mCompileSuccessionTask) {
            mCompileSuccessionTask->filename = getCompiledExecutableName(editor->filename());
        }
        mCompilerManager->compile(editor->filename(),editor->fileEncoding(),rebuild);
        openCloseMessageSheet(true);
        ui->tabMessages->setCurrentWidget(ui->tabCompilerOutput);
        return true;
    }
    return false;
}

void MainWindow::runExecutable(const QString &exeName,const QString &filename)
{
    // Check if it exists
    if (!QFile(exeName).exists()) {
        if (ui->actionCompile_Run->isEnabled()) {
            if (QMessageBox::warning(this,tr("Confirm"),
                                     tr("Source file is not compiled.")
                                     +"<br /><br />"+tr("Compile now?"),
                    QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
                ui->actionCompile_Run->trigger();
                return;
            }
        } else {
            QMessageBox::critical(this,"Error",
                                  tr("Source file is not compiled."));
            return;
        }
    } else {
        if (!filename.isEmpty() && compareFileModifiedTime(filename,exeName)>=0) {
            if (ui->actionCompile_Run->isEnabled()) {
                if (QMessageBox::warning(this,tr("Confirm"),
                                         tr("Source file is more recent than executable.")
                                         +"<br /><br />"+tr("Recompile now?"),
                        QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
                    ui->actionCompile_Run->trigger();
                    return;
                }
            }
        }
    }
      // Pause programs if they contain a console
//          if devData.ConsolePause and ProgramHasConsole(FileToRun) then begin
//            if fUseRunParams then
//              Parameters := '"' + FileToRun + '" ' + fRunParams
//            else
//              Parameters := '"' + FileToRun + '"';

//            FileToRun := devDirs.Exec + 'ConsolePauser.exe';
//          end else begin
//            if fUseRunParams then
//              Parameters := fRunParams
//            else
//              Parameters := '';
//            FileToRun := FileToRun;
//          end;

//          if devData.MinOnRun then
//            Application.Minimize;
//          devExecutor.ExecuteAndWatch(FileToRun, Parameters, ExtractFilePath(fSourceFile),
//            True, UseInputFile,InputFile, INFINITE, RunTerminate);
//          MainForm.UpdateAppTitle;
//        end;
    mCompilerManager->run(exeName,"",QFileInfo(exeName).absolutePath());
}

void MainWindow::runExecutable()
{
    Editor * editor = mEditorList->getEditor();
    if (editor != NULL ) {
        if (editor->modified()) {
            if (!editor->save(false,false))
                return;
        }
        QString exeName= getCompiledExecutableName(editor->filename());
        runExecutable(exeName,editor->filename());
    }
}

void MainWindow::openCloseMessageSheet(bool open)
{
//    if Assigned(fReportToolWindow) then
//      Exit;
    if (mTabMessagesTogglingState)
        return;
    mTabMessagesTogglingState = true;
    auto action = finally([this]{
        mTabMessagesTogglingState = false;
    });
    // Switch between open and close
    if (open) {
        QList<int> sizes = ui->splitterMessages->sizes();
        int tabHeight = ui->tabMessages->tabBar()->height();
        ui->tabMessages->setMinimumHeight(tabHeight+5);
        int totalSize = sizes[0] + sizes[1];
        sizes[1] = mPreviousHeight;
        sizes[0] = std::max(1,totalSize - sizes[1]);
        ui->splitterMessages->setSizes(sizes);
    } else {
        QList<int> sizes = ui->splitterMessages->sizes();
        mPreviousHeight = sizes[1];
        int totalSize = sizes[0] + sizes[1];
        int tabHeight = ui->tabMessages->tabBar()->height();
        ui->tabMessages->setMinimumHeight(tabHeight);
        sizes[1] = tabHeight;
        sizes[0] = std::max(1,totalSize - sizes[1]);
        ui->splitterMessages->setSizes(sizes);
    }
    QSplitterHandle* handle = ui->splitterMessages->handle(1);
    handle->setEnabled(open);
    int idxClose = ui->tabMessages->indexOf(ui->tabClose);
    ui->tabMessages->setTabVisible(idxClose,open);
    mTabMessagesTogglingState = false;
}


void MainWindow::on_actionNew_triggered()
{
    try {
        Editor * editor=mEditorList->newEditor("",ENCODING_AUTO_DETECT,false,true);
        editor->activate();
        updateForEncodingInfo();
    }  catch (FileError e) {
        QMessageBox::critical(this,tr("Error"),e.reason());
    }
}

void MainWindow::on_EditorTabsLeft_tabCloseRequested(int index)
{
    Editor* editor = mEditorList->getEditor(index);
    mEditorList->closeEditor(editor);
}

void MainWindow::on_actionOpen_triggered()
{
    try {
        QString selectedFileFilter = pSystemConsts->defaultFileFilter();
        QStringList files = QFileDialog::getOpenFileNames(pMainWindow,
            tr("Open"), QString(), pSystemConsts->defaultFileFilters().join(";;"),
            &selectedFileFilter);
        openFiles(files);
    }  catch (FileError e) {
        QMessageBox::critical(this,tr("Error"),e.reason());
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (!mEditorList->closeAll(false)) {
        event->ignore();
        return ;
    }

    delete mEditorList;
    event->accept();
    return;
}

void MainWindow::on_actionSave_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor != NULL) {
        try {
            editor->save();
        } catch(FileError e) {
            QMessageBox::critical(this,tr("Error"),e.reason());
        }
    }    
}

void MainWindow::on_actionSaveAs_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor != NULL) {
        try {
            editor->saveAs();
        } catch(FileError e) {
            QMessageBox::critical(this,tr("Error"),e.reason());
        }
    }
}

void MainWindow::on_actionOptions_triggered()
{
    SettingsDialog settingsDialog;
    settingsDialog.exec();
}

void MainWindow::onCompilerSetChanged(int index)
{
    if (index<0)
        return;
    pSettings->compilerSets().setDefaultIndex(index);
    pSettings->compilerSets().saveDefaultIndex();
}

void MainWindow::onCompileLog(const QString &msg)
{
    ui->txtCompilerOutput->appendPlainText(msg);
}

void MainWindow::onCompileIssue(PCompileIssue issue)
{
    ui->tableIssues->addIssue(issue);

    // Update tab caption
//    if CompilerOutput.Items.Count = 1 then
//      CompSheet.Caption := Lang[ID_SHEET_COMP] + ' (' + IntToStr(CompilerOutput.Items.Count) + ')';

    if (issue->type == CompileIssueType::Error || issue->type ==
            CompileIssueType::Warning) {
        Editor* e = mEditorList->getOpenedEditorByFilename(issue->filename);
        if (e!=nullptr && (issue->line>0)) {
            int line = issue->line;
            if (line > e->lines()->count())
                return;
            int col = std::min(issue->column,e->lines()->getString(line-1).length()+1);
            if (col < 1)
                col = e->lines()->getString(line-1).length()+1;
            e->addSyntaxIssues(line,col,issue->endColumn,issue->type,issue->description);
        }
    }
}

void MainWindow::onCompileFinished()
{
    // Update tab caption
    int i = ui->tabMessages->indexOf(ui->tabIssues);
    if (i==-1)
        return;
    ui->tabMessages->setTabText(i, tr("Issues") +
                                QString(" (%1)").arg(ui->tableIssues->model()->rowCount()));

    // Close it if there's nothing to show
    if (mCheckSyntaxInBack) {
      // check syntax in back, don't change message panel
    } else if (
        (ui->tableIssues->count() == 0)
//        and (ResourceOutput.Items.Count = 0)
//        and devData.AutoCloseProgress
               ) {
        openCloseMessageSheet(false);
        // Or open it if there is anything to show
    } else {
        if (ui->tableIssues->count() > 0) {
            if (ui->tabMessages->currentIndex() != i) {
                ui->tabMessages->setCurrentIndex(i);
                mMessageControlChanged = false;
            }
//      end else if (ResourceOutput.Items.Count > 0) then begin
//        if MessageControl.ActivePage <> ResSheet then begin
//          MessageControl.ActivePage := ResSheet;
//          fMessageControlChanged := False;
//        end;
//      end;
            openCloseMessageSheet(true);
        }
    }

    Editor * e = mEditorList->getEditor();
    if (e!=nullptr) {
        e->invalidate();
    }

    // Jump to problem location, sorted by significance
    if ((mCompilerManager->compileErrorCount() > 0) && (!mCheckSyntaxInBack)) {
        // First try to find errors
        for (int i=0;i<ui->tableIssues->count();i++) {
            PCompileIssue issue = ui->tableIssues->issue(i);
            if (issue->type == CompileIssueType::Error) {
                ui->tableIssues->selectRow(i);
                QModelIndex index =ui->tableIssues->model()->index(i,0);
                ui->tableIssues->doubleClicked(index);
            }
        }

        // Then try to find warnings
        for (int i=0;i<ui->tableIssues->count();i++) {
            PCompileIssue issue = ui->tableIssues->issue(i);
            if (issue->type == CompileIssueType::Warning) {
                ui->tableIssues->selectRow(i);
                QModelIndex index =ui->tableIssues->model()->index(i,0);
                ui->tableIssues->doubleClicked(index);
            }
        }
        // Then try to find anything with a line number...
//      for I := 0 to CompilerOutput.Items.Count - 1 do begin
//        if not SameStr(CompilerOutput.Items[I].Caption, '') then begin
//          CompilerOutput.Selected := CompilerOutput.Items[I];
//          CompilerOutput.Selected.MakeVisible(False);
//          CompilerOutputDblClick(CompilerOutput);
//          Exit;
//        end;
//      end;

      // Then try to find a resource error
//      if ResourceOutput.Items.Count > 0 then begin
//        ResourceOutput.Selected := ResourceOutput.Items[0];
//        ResourceOutput.Selected.MakeVisible(False);
//        CompilerOutputDblClick(ResourceOutput);
//      end;
    } else {
        if (mCompileSuccessionTask) {
            switch (mCompileSuccessionTask->type) {
            case MainWindow::CompileSuccessionTaskType::Run:
                runExecutable(mCompileSuccessionTask->filename);
            }
        }
    }
    mCheckSyntaxInBack=false;
}

void MainWindow::onCompileErrorOccured(const QString &reason)
{
    QMessageBox::critical(this,tr("Compile Failed"),reason);
}

void MainWindow::onRunErrorOccured(const QString &reason)
{
    QMessageBox::critical(this,tr("Run Failed"),reason);
}

void MainWindow::onRunFinished()
{
    qDebug()<<"run finished";
}

void MainWindow::on_actionCompile_triggered()
{
    mCompileSuccessionTask.reset();
    compile();
}

void MainWindow::on_actionRun_triggered()
{
    runExecutable();
}

void MainWindow::on_actionUndo_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor != NULL ) {
        editor->undo();
    }
}

void MainWindow::on_actionRedo_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor != NULL ) {
        editor->redo();
    }
}

void MainWindow::on_actionCut_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor != NULL ) {
        editor->cutToClipboard();
    }
}

void MainWindow::on_actionSelectAll_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor != NULL ) {
        editor->selectAll();
    }
}

void MainWindow::on_actionCopy_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor != NULL ) {
        editor->copyToClipboard();
    }
}

void MainWindow::on_actionPaste_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor != NULL ) {
        editor->pasteFromClipboard();
    }
}

void MainWindow::on_actionIndent_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor != NULL ) {
        editor->tab();
    }
}

void MainWindow::on_actionUnIndent_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor != NULL ) {
        editor->untab();
    }
}

void MainWindow::on_actionToggleComment_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor != NULL ) {
        editor->toggleComment();
    }
}

void MainWindow::on_actionUnfoldAll_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor != NULL ) {
        //editor->clearFolds();
    }
}

void MainWindow::on_actionFoldAll_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor != NULL ) {
        //editor->clearFolds();
        //editor->foldAll();
    }
}

void MainWindow::on_tableIssues_doubleClicked(const QModelIndex &index)
{
    PCompileIssue issue = ui->tableIssues->issue(index);
    if (!issue)
        return;

    Editor * editor = mEditorList->getEditorByFilename(issue->filename);
    if (editor == nullptr)
        return;

    editor->setCaretPositionAndActivate(issue->line,issue->column);
}

void MainWindow::on_actionEncode_in_ANSI_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor == nullptr)
        return;
    try {
        editor->setEncodingOption(ENCODING_SYSTEM_DEFAULT);
    } catch(FileError e) {
        QMessageBox::critical(this,tr("Error"),e.reason());
    }
}

void MainWindow::on_actionEncode_in_UTF_8_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor == nullptr)
        return;
    try {
        editor->setEncodingOption(ENCODING_UTF8);
    } catch(FileError e) {
        QMessageBox::critical(this,tr("Error"),e.reason());
    }
}

void MainWindow::on_actionAuto_Detect_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor == nullptr)
        return;
    editor->setEncodingOption(ENCODING_AUTO_DETECT);
}

void MainWindow::on_actionConvert_to_ANSI_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor == nullptr)
        return;
    if (QMessageBox::warning(this,tr("Confirm Convertion"),
                   tr("The editing file will be saved using %1 encoding. <br />This operation can't be reverted. <br />Are you sure to continue?")
                   .arg(QString(QTextCodec::codecForLocale()->name())),
                   QMessageBox::Yes, QMessageBox::No)!=QMessageBox::Yes)
        return;
    editor->convertToEncoding(ENCODING_SYSTEM_DEFAULT);

}

void MainWindow::on_actionConvert_to_UTF_8_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor == nullptr)
        return;
    if (QMessageBox::warning(this,tr("Confirm Convertion"),
                   tr("The editing file will be saved using %1 encoding. <br />This operation can't be reverted. <br />Are you sure to continue?")
                   .arg(ENCODING_UTF8),
                   QMessageBox::Yes, QMessageBox::No)!=QMessageBox::Yes)
        return;
    editor->convertToEncoding(ENCODING_UTF8);
}

void MainWindow::on_tabMessages_tabBarClicked(int index)
{
    if (index == ui->tabMessages->currentIndex()) {
        openCloseMessageSheet(!ui->splitterMessages->handle(1)->isEnabled());
    }
}

void MainWindow::on_tabMessages_currentChanged(int index)
{
    mMessageControlChanged = true;
    int idxClose = ui->tabMessages->indexOf(ui->tabClose);
    if (index == idxClose) {
        openCloseMessageSheet(false);
    } else {
        openCloseMessageSheet(true);
    }
}

void MainWindow::on_tabMessages_tabBarDoubleClicked(int index)
{

}

void MainWindow::on_actionCompile_Run_triggered()
{
    mCompileSuccessionTask = std::make_shared<CompileSuccessionTask>();
    mCompileSuccessionTask->type = CompileSuccessionTaskType::Run;
    compile();
}

void MainWindow::on_actionRebuild_triggered()
{
    mCompileSuccessionTask.reset();
    compile(true);
}

void MainWindow::on_actionStop_Execution_triggered()
{
    mCompilerManager->stopRun();
}
