package com.watergate.library.io;

import com.watergate.library.LockClientEnv;
import com.watergate.library.LockControlClient;
import com.watergate.library.LockControlClient.ELockResult;
import com.watergate.library.LockControlClient.EResourceType;
import com.watergate.library.LockControlException;
import com.watergate.library.ObjectState.StateException;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;

/**
 * Created by subho on 17/10/16.
 */
public class PriorityFileInputStream extends FileInputStream {
	private LockControlClient lockClient;
	private short priority;
	private String lockname = null;
	private int currentLockCount = 0;

	public PriorityFileInputStream(String name, short priority) throws
			LockControlException, StateException, IOException {
		super(name);
		File f = new File(name);
		setup(priority, f.getCanonicalPath());
	}

	public PriorityFileInputStream(File file, short priority) throws
			LockControlException, StateException, IOException {
		super(file);
		setup(priority, file.getCanonicalPath());
	}


	public short getPriority() {
		return priority;
	}

	private void setup(short priority, String filename) throws
			LockControlException, StateException {
		this.priority = priority;
		this.lockClient = LockClientEnv.getLockClient();

		lockname = lockClient.findLockByName(filename, EResourceType.getValue
				(EResourceType.FS));
		lockClient.register_thread(lockname);
	}

	@Override
	public int read() throws IOException {
		try {
			if (lockname != null && !lockname.isEmpty()) {
				ELockResult r = lockClient.getLock(lockname, priority, 1);
				if (r == ELockResult.Locked) {
					try {
						return super.read();
					} finally {
						lockClient.release(lockname, priority);
					}
				} else {
					throw new LockControlException(String.format("Error " +
							"reading " +
							" from " +
							"locked file. [result=%s]", r.name()));
				}
			} else {
				return super.read();
			}
		} catch (Throwable t) {
			throw new IOException(t);
		}
	}

	@Override
	public int read(byte[] b) throws IOException {
		try {
			if (lockname != null && !lockname.isEmpty()) {
				currentLockCount = 0;
				try {
					int quota = (int) lockClient.getQuota(lockname);
					int read = 0;
					while (read < b.length) {
						int rem = b.length - read;
						if (rem > quota) {
							rem = quota;
						}
						int r = readBlock(b, read, rem);
						if (r < rem)
							break;
						read += r;
					}
					return read;
				} finally {
					for (int ii = 0; ii < currentLockCount; ii++) {
						lockClient.release(lockname, priority);
					}
				}

			} else {
				return super.read(b);
			}
		} catch (Throwable t) {
			throw new IOException(t);
		}
	}

	@Override
	public int read(byte[] b, int off, int len) throws IOException {
		try {
			if (lockname != null && !lockname.isEmpty()) {
				currentLockCount = 0;
				try {
					int quota = (int) lockClient.getQuota(lockname);
					int read = 0;

					while (read < len) {
						int rem = len - read;
						if (rem > quota) {
							rem = quota;
						}
						int r = readBlock(b, (off + read), rem);
						if (r < rem)
							break;
						read += r;
					}
					return read;
				} finally {
					for (int ii = 0; ii < currentLockCount; ii++) {
						lockClient.release(lockname, priority);
					}
				}
			} else {
				return super.read(b, off, len);
			}
		} catch (Throwable t) {
			throw new IOException(t);
		}
	}

	private int readBlock(byte[] b, int off, int len) throws IOException, LockControlException {
		ELockResult r = lockClient.getLock(lockname, priority, len);
		if (r == ELockResult.Locked) {
			currentLockCount++;
			return super.read(b, off, len);
		} else {
			throw new LockControlException(String.format("Error " +
					"reading " +
					" from " +
					"locked file. [result=%s]", r.name()));
		}
	}
}
