import { useState } from "preact/hooks";
import { DetectorPage } from "./components/detector/detector-page";
import { FlockyouPage } from "./components/flockyou/flockyou-page";
import { FoxhunterPage } from "./components/foxhunter/foxhunter-page";
import { Header } from "./components/layout/header";
import { PageShell } from "./components/layout/page-shell";
import { TabBar } from "./components/layout/tab-bar";
import { ToastProvider } from "./components/shared/toast";
import { SkyspyPage } from "./components/skyspy/skyspy-page";
import { SystemPage } from "./components/system/system-page";

const TABS = ["SYSTEM", "DETECT", "FOX", "FLOCK", "SKY"] as const;

export function App() {
  const [activeTab, setActiveTab] = useState(0);

  return (
    <ToastProvider>
      <Header />
      <TabBar tabs={TABS} active={activeTab} onSelect={setActiveTab} />
      <PageShell>
        {activeTab === 0 && <SystemPage />}
        {activeTab === 1 && <DetectorPage />}
        {activeTab === 2 && <FoxhunterPage />}
        {activeTab === 3 && <FlockyouPage />}
        {activeTab === 4 && <SkyspyPage />}
      </PageShell>
    </ToastProvider>
  );
}
