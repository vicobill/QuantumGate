// This file is part of the QuantumGate project. For copyright and
// licensing information refer to the license file(s) in the project root.

#include "pch.h"
#include "TestApp.h"
#include "CPeerAccessDlg.h"

IMPLEMENT_DYNAMIC(CPeerAccessDlg, CDialogBase)

CPeerAccessDlg::CPeerAccessDlg(CWnd* pParent /*=nullptr*/)
	: CDialogBase(IDD_PEER_ACCESS_DIALOG, pParent)
{}

CPeerAccessDlg::~CPeerAccessDlg()
{}

void CPeerAccessDlg::SetAccessManager(QuantumGate::Access::Manager* am) noexcept
{
	m_AccessManager = am;
}

void CPeerAccessDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogBase::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CPeerAccessDlg, CDialogBase)
	ON_EN_CHANGE(IDC_UUID, &CPeerAccessDlg::OnEnChangeUuid)
	ON_CBN_SELCHANGE(IDC_ACCESS_COMBO, &CPeerAccessDlg::OnCbnSelchangeAccessCombo)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_PEER_LIST, &CPeerAccessDlg::OnLvnItemchangedPeerList)
	ON_BN_CLICKED(IDC_ADD_PEER, &CPeerAccessDlg::OnBnClickedAddPeer)
	ON_BN_CLICKED(IDC_REMOVE_PEER, &CPeerAccessDlg::OnBnClickedRemovePeer)
	ON_BN_CLICKED(IDC_BROWSE, &CPeerAccessDlg::OnBnClickedBrowse)
	ON_CBN_SELCHANGE(IDC_DEFAULT_ACCESS_COMBO, &CPeerAccessDlg::OnCbnSelChangeDefaultAccessCombo)
END_MESSAGE_MAP()

BOOL CPeerAccessDlg::OnInitDialog()
{
	CDialogBase::OnInitDialog();

	// Init peer access combo
	const auto tcombo = (CComboBox*)GetDlgItem(IDC_ACCESS_COMBO);
	auto pos = tcombo->AddString(L"Yes");
	tcombo->SetItemData(pos, TRUE);
	pos = tcombo->AddString(L"No");
	tcombo->SetItemData(pos, FALSE);

	// Init default access combo
	const auto dacombo = (CComboBox*)GetDlgItem(IDC_DEFAULT_ACCESS_COMBO);
	pos = dacombo->AddString(L"Allowed");
	dacombo->SetItemData(pos, TRUE);
	pos = dacombo->AddString(L"Not Allowed");
	dacombo->SetItemData(pos, FALSE);

	const auto da = m_AccessManager->GetPeerAccessDefault();
	if (da == Access::PeerAccessDefault::Allowed) dacombo->SelectString(0, L"Allowed");
	else dacombo->SelectString(0, L"Not Allowed");

	// Init filter list
	auto slctrl = (CListCtrl*)GetDlgItem(IDC_PEER_LIST);
	slctrl->SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
	slctrl->InsertColumn(0, _T("UUID"), LVCFMT_LEFT, GetApp()->GetScaledWidth(225));
	slctrl->InsertColumn(1, _T("Public Key"), LVCFMT_LEFT, GetApp()->GetScaledWidth(75));
	slctrl->InsertColumn(2, _T("Allowed"), LVCFMT_LEFT, GetApp()->GetScaledWidth(75));

	UpdatePeerList();
	UpdateControls();

	return TRUE;
}

void CPeerAccessDlg::UpdatePeerList() noexcept
{
	auto slctrl = (CListCtrl*)GetDlgItem(IDC_PEER_LIST);
	slctrl->DeleteAllItems();

	const auto result = m_AccessManager->GetAllPeers();
	if (result.Succeeded())
	{
		for (auto& peer : *result)
		{
			CString haspubkey = L"No";
			if (!peer.PublicKey.IsEmpty()) haspubkey = L"Yes";

			CString allowed = L"No";
			if (peer.AccessAllowed) allowed = L"Yes";

			auto pos = slctrl->InsertItem(0, peer.UUID.GetString().c_str());
			if (pos != -1)
			{
				slctrl->SetItemText(pos, 1, haspubkey);
				slctrl->SetItemText(pos, 2, allowed);
			}
		}
	}
	else AfxMessageBox(L"Could not get peers.", MB_ICONERROR);
}

void CPeerAccessDlg::UpdateControls() noexcept
{
	const auto puuid = GetTextValue(IDC_UUID);
	const auto sel = ((CComboBox*)GetDlgItem(IDC_ACCESS_COMBO))->GetCurSel();

	if (puuid.GetLength() > 0 && sel != -1)
	{
		GetDlgItem(IDC_ADD_PEER)->EnableWindow(true);
	}
	else
	{
		GetDlgItem(IDC_ADD_PEER)->EnableWindow(false);
	}

	const auto slctrl = (CListCtrl*)GetDlgItem(IDC_PEER_LIST);
	if (slctrl->GetSelectedCount() > 0)
	{
		GetDlgItem(IDC_REMOVE_PEER)->EnableWindow(true);
	}
	else
	{
		GetDlgItem(IDC_REMOVE_PEER)->EnableWindow(false);
	}
}

void CPeerAccessDlg::OnEnChangeUuid()
{
	UpdateControls();
}

void CPeerAccessDlg::OnCbnSelchangeAccessCombo()
{
	UpdateControls();
}

void CPeerAccessDlg::OnLvnItemchangedPeerList(NMHDR *pNMHDR, LRESULT *pResult)
{
	UpdateControls();

	*pResult = 0;
}

void CPeerAccessDlg::OnBnClickedAddPeer()
{
	Access::PeerSettings pas;
	if (QuantumGate::UUID::TryParse(GetTextValue(IDC_UUID).GetString(), pas.UUID))
	{
		const auto sel = ((CComboBox*)GetDlgItem(IDC_ACCESS_COMBO))->GetCurSel();
		const auto allowed = ((CComboBox*)GetDlgItem(IDC_ACCESS_COMBO))->GetItemData(sel) == TRUE ? true : false;
		pas.AccessAllowed = allowed;

		auto path = GetTextValue(IDC_PUB_PEM);
		if (path.GetLength() > 0)
		{
			if (!GetApp()->LoadKey(path.GetString(), pas.PublicKey))
			{
				return;
			}
		}
		else
		{
			const auto ret = AfxMessageBox(L"No public key file was specified; the peer may fail authentication. Add the peer without a public key?", MB_ICONQUESTION | MB_YESNO);
			if (ret == IDNO) return;
		}

		if (m_AccessManager->AddPeer(std::move(pas)).Failed())
		{
			AfxMessageBox(L"Couldn't add peer.", MB_ICONERROR);
		}
		else
		{
			((CComboBox*)GetDlgItem(IDC_ACCESS_COMBO))->SetCurSel(-1);
			SetValue(IDC_UUID, L"");
			SetValue(IDC_PUB_PEM, L"");

			UpdatePeerList();
			UpdateControls();
		}
	}
	else AfxMessageBox(L"Invalid UUID.", MB_ICONERROR);
}

void CPeerAccessDlg::OnBnClickedRemovePeer()
{
	const auto slctrl = (CListCtrl*)GetDlgItem(IDC_PEER_LIST);
	if (slctrl->GetSelectedCount() > 0)
	{
		auto position = slctrl->GetFirstSelectedItemPosition();
		const auto pos = slctrl->GetNextSelectedItem(position);
		const auto uuid = slctrl->GetItemText(pos, 0);

		if (m_AccessManager->RemovePeer(QuantumGate::UUID(uuid.GetString())) != QuantumGate::ResultCode::Succeeded)
		{
			AfxMessageBox(L"Couldn't remove the peer.", MB_ICONERROR);
		}
		else
		{
			UpdatePeerList();
			UpdateControls();
		}
	}
}

void CPeerAccessDlg::OnBnClickedBrowse()
{
	const auto path = GetApp()->BrowseForFile(GetSafeHwnd(), false);
	if (path)
	{
		SetValue(IDC_PUB_PEM, *path);
	}
}

void CPeerAccessDlg::OnCbnSelChangeDefaultAccessCombo()
{
	const auto sel = ((CComboBox*)GetDlgItem(IDC_DEFAULT_ACCESS_COMBO))->GetCurSel();
	if (sel != CB_ERR)
	{
		const auto da = ((CComboBox*)GetDlgItem(IDC_DEFAULT_ACCESS_COMBO))->GetItemData(sel) == TRUE ?
			Access::PeerAccessDefault::Allowed : Access::PeerAccessDefault::NotAllowed;
		m_AccessManager->SetPeerAccessDefault(da);
	}
}
