package com.watergate.library;

/**
 * Created by subho on 17/10/16.
 */
public class LockControlException extends Exception {
	private static final String _PREFIX_ = "Lock Control Error : ";

	public LockControlException(String mesg) {
		super(String.format("%s %s", _PREFIX_, mesg));
	}

	public LockControlException(String mesg, Throwable t) {
		super(String.format("%s %s", _PREFIX_, mesg), t);
	}
}
