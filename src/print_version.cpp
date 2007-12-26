#include <iostream>
#include <mpolib/mpo_xml.h>
#include <mpolib/mpo_fileio.h>
#include <mpolib/mpo_misc.h>

int main(int argc, char **argv)
{
	mpo_io *io = mpo_open("DaphneManifest.xml", MPO_OPEN_READONLY);
	if (io)
	{
		string strErrMsg;
		bool bRes;
		mpo_xml x;
		mpo_buf buf;
		buf.alloc(io->size);
		mpo_read(buf.data(), buf.size(), NULL, io);
		bRes = x.parse(buf.data(), buf.size(), strErrMsg);
		if (bRes)
		{
			const xml_element *e = x.get_parsed_element();
			const xml_element *pElVer = x.find_child(e, "version", false);
			const xml_element *pChild = x.find_child(pElVer, "major", false);
			cout << pChild->text << ".";
			pChild = x.find_child(pElVer, "minor", false);
			cout << pChild->text << ".";
			pChild = x.find_child(pElVer, "build", false);
			cout << pChild->text;
		}
		else
		{
			cerr << "ERROR IN PRINT_VERSION : XML parse failed!" << endl;
		}
		mpo_close(io);
	}
	return 0;
}
