void fill_local_ocdb()
{
  AliceO2::CDB::Manager* cdb = AliceO2::CDB::Manager::Instance();
  cdb->setDefaultStorage("local://O2CDB");
  TH1F* h = 0;
  for (int run = 2000; run < 2100; run++) {
    cdb->setRun(run);
    h = new TH1F(Form("histo for %d run", run), "gauss", 100, -5, 5);
    h->FillRandom("gaus", 1000);
    AliceO2::CDB::ConditionId* id = new AliceO2::CDB::ConditionId("DET/Calib/Histo", run, run, 1, 0);
    AliceO2::CDB::ConditionMetaData* md = new AliceO2::CDB::ConditionMetaData();
    cdb->putObject(h, *id, md);
    h = 0;
  }
}
