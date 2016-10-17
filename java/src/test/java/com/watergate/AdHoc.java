package com.watergate;

import com.watergate.library.LockClientEnv;
import com.watergate.library.LockControlManager;
import com.watergate.library.LockEnv;
import com.watergate.library.io.PriorityFileOutputStream;

import java.io.FileInputStream;

/**
 * Created by subho on 17/10/16.
 */
public class AdHoc {
	private static final String CONFIG_FILE =
			"/work/dev/wookler/watergate/test/data/test-sem-conf.json";
	private static final String CONTROL_DEF_CONFIG_PATH =
			"/configuration/control/def";
	private static final String CONTROL_CONFIG_PATH = "/configuration/control";
	private static final String DATA_FILE = "/work/dev/wookler/watergate/test/data/fs-data.txt";

	public static void main(String[] args) {
		try {
			LockEnv.createEnv(CONFIG_FILE, "ADHOC");

			LockControlManager manager = new LockControlManager();
			manager.create(CONTROL_CONFIG_PATH);

			LockClientEnv.createEnv(CONTROL_DEF_CONFIG_PATH);
			StringBuffer data = new StringBuffer();
			FileInputStream fis = new FileInputStream(DATA_FILE);
			while (true) {
				byte[] buff = new byte[1024];
				int r = fis.read(buff);
				if (r > 0) {
					String s = new String(buff, 0, r, "UTF-8");
					data.append(s);
					if (r < 1024) {
						break;
					}
				} else {
					break;
				}
			}
			fis.close();
			if (data.length() > 0) {
				PriorityFileOutputStream fos = new PriorityFileOutputStream
						("/tmp/output.txt", false, (short) 2);
				fos.write(data.toString().getBytes("UTF-8"));
				fos.close();
			}
			manager.dispose();
			LockClientEnv.shutdown();

		} catch (Throwable t) {
			t.printStackTrace();
		}
	}
}
