package com.watergate.library.io;

import com.watergate.library.utils.LogUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;
import java.io.FileInputStream;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.nio.file.StandardCopyOption;

/**
 * Created by subho on 17/10/16.
 */
public class PriorityTestRunner implements Runnable {
	private static final Logger _log = LoggerFactory.getLogger
			(PriorityTestRunner.class);
	private static final String DATA_FILE = "../test/data/fs-data.txt";

	private short priority;
	private int index;
	private int cycles;

	public PriorityTestRunner(short priority, int index, int cycles) {
		this.priority = priority;
		this.index = index;
		this.cycles = cycles;
	}

	@Override
	public void run() {
		LogUtils.mesg(getClass(), String.format("Launching runner. " +
				"[priority=%d][index=%d]", priority, index), _log);
		try {
			String runid = getClass().getSimpleName() + "__" + index;
			String readfile = String.format("/tmp/%s/input/%s.txt", getClass
					().getSimpleName(), runid);
			Files.copy(Paths.get(DATA_FILE), Paths.get(readfile),
					StandardCopyOption.REPLACE_EXISTING);
			StringBuffer data = new StringBuffer();
			FileInputStream fis = new FileInputStream(readfile);
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
			long write_time = 0;
			long read_time = 0;
			for (int ii = 0; ii < cycles; ii++) {
				String outfile = String.format("/tmp/%s/output/%s_%d.txt",
						getClass().getSimpleName(), runid, ii);
				File of = new File(outfile);
				if (of.exists()) {
					of.delete();
				}

				if (data.length() > 0) {
					PriorityFileOutputStream fos = new PriorityFileOutputStream
							(outfile, false, priority);
					long ts = System.currentTimeMillis();
					fos.write(data.toString().getBytes("UTF-8"));
					write_time += (System.currentTimeMillis() - ts);
					fos.close();
				}

				PriorityFileInputStream pis = new PriorityFileInputStream
						(outfile, priority);
				StringBuffer read = new StringBuffer();
				long ts = System.currentTimeMillis();
				while (true) {
					byte[] buff = new byte[10024];
					int r = pis.read(buff);
					if (r > 0) {
						String s = new String(buff, 0, r, "UTF-8");
						read.append(s);
						if (r < 1024) {
							break;
						}
					} else {
						break;
					}
				}
				read_time += (System.currentTimeMillis() - ts);
				pis.close();
			}
			LogUtils.mesg(getClass(), String.format("Finished run " +
							"[index=%d][priority=%d][write time=%d][read time=%d]",
					index, priority, write_time, read_time));
		} catch (Throwable t) {
			LogUtils.stacktrace(getClass(), t, _log);
			LogUtils.error(getClass(), t, _log);
		}
	}
}
